#include "ubuntuprojectmigrationwizard.h"

#include "../ubuntuclicktool.h"
#include "../ubuntubzr.h"
#include "../ubuntuscopefinalizer.h"
#include "../ubuntuconstants.h"

#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakenodes.h>

#include <qtsupport/profilereader.h>

#include <QCheckBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QRegularExpression>
#include <QTextStream>

namespace Ubuntu {
namespace Internal {

static bool createFileFromTemplate (const QString &templateFile, const QString &targetFile, const QMap<QString,QString> replacements)
{
    QString templateText;
    QFile inFile(templateFile);
    if(!inFile.open(QIODevice::ReadOnly))
        return false;

    QFile outFile(targetFile);
    QFileInfo outFileInfo(targetFile);

    QDir d;
    if(!d.mkpath(outFileInfo.absolutePath()))
        return false;

    if(!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    QTextStream in(&inFile);
    templateText = in.readAll();

    for(auto i = replacements.constBegin(); i != replacements.constEnd(); i++) {
        templateText.replace(QString::fromLatin1("%%1%").arg(i.key()),i.value());
        templateText.replace(QString::fromLatin1("%%1:l%").arg(i.key()),i.value().toLower());
    }

    QTextStream out(&outFile);
    out<<templateText;

    inFile.close();
    outFile.close();

    return true;
}


UbuntuProjectMigrationWizard::UbuntuProjectMigrationWizard(QmakeProjectManager::QmakeProject *project, QWidget *parent) :
    Utils::Wizard(parent),
    m_project(project)
{
    const int selectTargetPageId = addPage(new UbuntuSelectSubProjectsPage);
    const int projectsInfoPageId = addPage(new UbuntuProjectDetailsPage);

    Utils::WizardProgress *progress = wizardProgress();

    progress->item(selectTargetPageId)->setTitle(tr("Targets"));
    progress->item(projectsInfoPageId)->setTitle(tr("Details"));

    setMinimumSize(800,600);
}

QmakeProjectManager::QmakeProject *UbuntuProjectMigrationWizard::project() const
{
    return m_project;
}

QStringList UbuntuProjectMigrationWizard::relevantTargets() const
{
    QStringList result;
    QList<QmakeProjectManager::QmakeProFileNode *> nodes = m_project->allProFiles();
    foreach(const QmakeProjectManager::QmakeProFileNode * node, nodes) {
        if(node->projectType() == QmakeProjectManager::ApplicationTemplate)
            result += node->targetInformation().target;
    }

    return result;
}

QStringList UbuntuProjectMigrationWizard::selectedTargets() const
{
    QStringList result;
    foreach(const QString &targetName, relevantTargets()) {
        if(field(QStringLiteral("addTarget_")+targetName).toBool())
            result += targetName;
    }

    return result;
}

QString UbuntuProjectMigrationWizard::maintainer() const
{
    return field(QStringLiteral("maintainer")).toString();
}

QString UbuntuProjectMigrationWizard::domain() const
{
    return field(QStringLiteral("domain")).toString();
}

QString UbuntuProjectMigrationWizard::framework() const
{
    return field(QStringLiteral("framework")).toString();
}

void UbuntuProjectMigrationWizard::doMigrateProject(QmakeProjectManager::QmakeProject *project, QWidget *parent)
{
    UbuntuProjectMigrationWizard wiz(project,parent);
    if(wiz.exec() == QDialog::Accepted) {
        bool multiTargetProject = project->rootQmakeProjectNode()->projectType() == QmakeProjectManager::SubDirsTemplate;

        QStringList hookTargets = wiz.selectedTargets();

        //contains all old paths mapped to the path they should have inthe click package
        QMap<QString,QString> mapPaths;


        QList<QmakeProjectManager::QmakeProFileNode *> nodes = project->allProFiles();
        foreach(QmakeProjectManager::QmakeProFileNode * node, nodes) {

            if(node->projectType() == QmakeProjectManager::SubDirsTemplate)
                continue;

            QtSupport::ProFileReader *reader = project->createProFileReader(node);

            OnScopeExit {
                qDebug()<<"Destroying reader";
                project->destroyProFileReader(reader);
            };

            bool canRead = true;
            //setup the file reader correctly

            if (ProFile *pro = reader->parsedProFile(node->path())) {
                if(!reader->accept(pro, QMakeEvaluator::LoadAll)) {
                    canRead = false;
                }
                pro->deref();
            } else {
                canRead = false;
            }

            if(canRead) {
                QString targetInstallPath;

                node->setProVariable(QStringLiteral("CONFIG"),
                                     QStringList()<<QStringLiteral("ubuntu-click"),
                                     QString(),
                                     QmakeProjectManager::Internal::ProWriter::AppendValues
                                     | QmakeProjectManager::Internal::ProWriter::AppendOperator);

                node->setProVariable(QStringLiteral("qt_install_libs"),
                                     QStringList()<<QStringLiteral("$$[QT_INSTALL_LIBS]"),
                                     QStringLiteral("ubuntu-click"));

                const auto projectType = node->projectType();

                //inspect installs
                QStringList installs = reader->values(QStringLiteral("INSTALLS"));
                bool hasInstallTarget = installs.contains(QStringLiteral("target"));
                bool isLikelyQmlPlugin = false;

                //we don't want to iterate over the install target again
                installs.removeAll(QStringLiteral("target"));

                if(hasInstallTarget) {
                    // try to map to the click way
                    const QString path = reader->value(QStringLiteral("target.path"));

                    if(projectType == QmakeProjectManager::LibraryTemplate) {
                        //this is probably a qml plugin, need to analyze more
                        foreach(const QString &install, installs) {
                            if(install == QStringLiteral("target"))
                                continue;

                            QStringList files = reader->values(install+QStringLiteral(".files"));
                            foreach(const QString &file,files) {
                                QFileInfo info(file);
                                if(info.completeBaseName() == QStringLiteral("qmldir")) {
                                    //-> we have a qml plugin
                                    //use the last directory of the path

                                    isLikelyQmlPlugin = true;

                                    QString inst_path = reader->value(install+QStringLiteral(".path"));
                                    QString prefix = inst_path.split(QStringLiteral("/")).last();

                                    targetInstallPath = QStringLiteral("/lib/$$basename(qt_install_libs)/qt5/qml/")+prefix;
                                    break;
                                }
                            }
                        }
                    }

                    //make sure all files targeted to that path go into the same direction
                    mapPaths.insert(path,targetInstallPath);
                }

                if(!hasInstallTarget || !isLikelyQmlPlugin) {
                    if(projectType == QmakeProjectManager::LibraryTemplate) {
                        targetInstallPath = QStringLiteral("/lib/$$basename(qt_install_libs)");
                    } else if(projectType == QmakeProjectManager::ApplicationTemplate) {
                        targetInstallPath = QStringLiteral("/lib/$$basename(qt_install_libs)/bin");
                    }
                }

                node->setProVariable(QStringLiteral("target.path"),
                                     QStringList()<<targetInstallPath,
                                     QStringLiteral("ubuntu-click"));

                if(!hasInstallTarget) {
                    node->setProVariable(QStringLiteral("INSTALLS"),
                                         QStringList()<<QStringLiteral("target"),
                                         QStringLiteral("ubuntu-click"),
                                         QmakeProjectManager::Internal::ProWriter::AppendValues
                                         | QmakeProjectManager::Internal::ProWriter::AppendOperator);
                }


                //now fix all the other installs
                foreach(const QString &install, installs) {
                    const QString path = reader->value(install+QStringLiteral(".path"));
                    QString mappedPath = path;

                    if(mapPaths.contains(path)) {
                        mappedPath = mapPaths[path];
                    } else {
                        //check if this installs into a subdirectory of a already changed path
                        bool found = false;
                        foreach(const QString &key,mapPaths.keys()) {
                            if(path.startsWith(key)) {
                                QString suffix = path;
                                suffix.remove(key);

                                mappedPath = mapPaths[key] + QStringLiteral("/") + suffix;
                                mapPaths.insert(path,mappedPath);

                                found = true;
                                break;
                            }
                        }

                        if(!found) {
                            mappedPath.remove(QStringLiteral("/usr"));
                        }
                    }


                    node->setProVariable(install+QStringLiteral(".path"),
                                         QStringList()<<mappedPath,
                                         QStringLiteral("ubuntu-click"));
                }

                //now add required files
                if(projectType == QmakeProjectManager::ApplicationTemplate) {

                    QFileInfo proFilePath(node->path());

                    QmakeProjectManager::TargetInformation targetInfo = node->targetInformation();
                    if(hookTargets.contains(targetInfo.target)) {
                        QMap<QString,QString> replacements;
                        replacements.insert(QStringLiteral("ProjectName"),project->displayName());
                        replacements.insert(QStringLiteral("ClickHookName"),targetInfo.target);
                        replacements.insert(QStringLiteral("ClickDomain"),wiz.field(QStringLiteral("domain")).toString());
                        replacements.insert(QStringLiteral("ClickMaintainer"),wiz.field(QStringLiteral("maintainer")).toString());

                        QString framework = wiz.field(QStringLiteral("framework")).toString();
                        QString aaPolicy  = UbuntuClickFrameworkProvider::instance()->frameworkPolicy(framework);
                        replacements.insert(QStringLiteral("ClickFrameworkVersion"),framework);
                        replacements.insert(QStringLiteral("ClickAAPolicyVersion"),aaPolicy.isEmpty() ? QStringLiteral("1.2") : aaPolicy);

                        if(!multiTargetProject) {
                            createFileFromTemplate(
                                        QString::fromLatin1("%1/templates/wizards/ubuntu/bin_app-qmake/manifest/manifest.json.in").arg(Constants::UBUNTU_RESOURCE_PATH),
                                        QString::fromLatin1("%1/manifest.json.in").arg(proFilePath.absolutePath()),
                                        replacements
                                        );

                            node->setProVariable(QStringLiteral("UBUNTU_MANIFEST_FILE"),QStringList()<<QStringLiteral("manifest.json.in"),QStringLiteral("ubuntu-click"));
                            node->setProVariable(QStringLiteral("CLICK_ARCH"),QStringList()<<QStringLiteral("$$system(dpkg-architecture -qDEB_HOST_ARCH)"),QStringLiteral("ubuntu-click"));
                            node->setProVariable(QStringLiteral("manifest.target"),QStringList()<<QStringLiteral("manifest.json"),QStringLiteral("ubuntu-click"));
                            node->setProVariable(QStringLiteral("manifest.commands"),QStringList()<<QStringLiteral("sed s/@CLICK_ARCH@/$$CLICK_ARCH/g $$PWD/manifest.json.in > $$manifest.target"),QStringLiteral("ubuntu-click"));
                            node->setProVariable(QStringLiteral("manifest.path"),QStringList()<<QStringLiteral("/"),QStringLiteral("ubuntu-click"));

                            node->setProVariable(QStringLiteral("QMAKE_EXTRA_TARGETS"),
                                                 QStringList()<<QStringLiteral("manifest"),
                                                 QStringLiteral("ubuntu-click"),
                                                 QmakeProjectManager::Internal::ProWriter::AppendValues
                                                 | QmakeProjectManager::Internal::ProWriter::AppendOperator);

                            node->setProVariable(QStringLiteral("PRE_TARGETDEPS"),
                                                 QStringList()<<QStringLiteral("$$manifest.target"),
                                                 QStringLiteral("ubuntu-click"),
                                                 QmakeProjectManager::Internal::ProWriter::AppendValues
                                                 | QmakeProjectManager::Internal::ProWriter::AppendOperator);

                            node->setProVariable(QStringLiteral("manifest_install.path"),
                                                 QStringList() << QStringLiteral("/"),
                                                 QStringLiteral("ubuntu-click"));

                            node->setProVariable(QStringLiteral("manifest_install.files"),
                                                 QStringList()<<QStringLiteral("manifest.json"),
                                                 QStringLiteral("ubuntu-click"));

                            node->setProVariable(QStringLiteral("INSTALLS"),
                                                 QStringList()<<QStringLiteral("manifest_install"),
                                                 QStringLiteral("ubuntu-click"),
                                                 QmakeProjectManager::Internal::ProWriter::AppendValues
                                                 | QmakeProjectManager::Internal::ProWriter::AppendOperator);

                            node->addFiles(QStringList()<<QStringLiteral("manifest.json.in"));
                        }

                        QString aaFileName   = QString::fromLatin1("%1.apparmor").arg(targetInfo.target);
                        QString deskFileName = QString::fromLatin1("%1.desktop").arg(targetInfo.target);
                        QString iconName     = QString::fromLatin1("%1.png").arg(targetInfo.target);

                        //add desktop and apparmor files
                        createFileFromTemplate(
                                    QString::fromLatin1("%1/templates/wizards/ubuntu/bin_app-qmake/appName/appName.apparmor").arg(Constants::UBUNTU_RESOURCE_PATH),
                                    QString::fromLatin1("%1/%2").arg(proFilePath.absolutePath()).arg(aaFileName),
                                    replacements
                                    );

                        createFileFromTemplate(
                                    QString::fromLatin1("%1/templates/wizards/ubuntu/bin_app-qmake/appName/appName.desktop").arg(Constants::UBUNTU_RESOURCE_PATH),
                                    QString::fromLatin1("%1/%2").arg(proFilePath.absolutePath()).arg(deskFileName),
                                    replacements
                                    );

                        QFile::copy(QString::fromLatin1("%1/templates/wizards/ubuntu/bin_app-qmake/appName/appName.png").arg(Constants::UBUNTU_RESOURCE_PATH),
                                    QString::fromLatin1("%1/%2").arg(proFilePath.absolutePath()).arg(iconName));

                        //make teh files visible
                        node->addFiles(QStringList()<< aaFileName << deskFileName << iconName);

                        node->setProVariable(QStringLiteral("appDeps.path"),
                                             QStringList() << QString::fromLatin1("/%1").arg(targetInfo.target),
                                             QStringLiteral("ubuntu-click"),
                                             QmakeProjectManager::Internal::ProWriter::AppendValues);

                        node->setProVariable(QStringLiteral("appDeps.files"),
                                             QStringList()<<aaFileName<<deskFileName<<iconName,
                                             QStringLiteral("ubuntu-click"),
                                             QmakeProjectManager::Internal::ProWriter::AppendValues);

                        node->setProVariable(QStringLiteral("INSTALLS"),
                                             QStringList()<<QStringLiteral("appDeps"),
                                             QStringLiteral("ubuntu-click"),
                                             QmakeProjectManager::Internal::ProWriter::AppendValues);
                    }
                }
            }
        }

        if(multiTargetProject) {
            //add manifest pro file
        }
    }
}

UbuntuSelectSubProjectsPage::UbuntuSelectSubProjectsPage(QWidget *parent) : QWizardPage(parent)
{
    QLabel *label = new QLabel(tr("<h2 style=\"text-align: center;\">Please select the applications you want to add to the manifest file</h2>"));
    label->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    setLayout(layout);
}

void UbuntuSelectSubProjectsPage::initializePage()
{
    QWizardPage::initializePage();
    UbuntuProjectMigrationWizard *wiz = qobject_cast<UbuntuProjectMigrationWizard *>(wizard());
    if(wiz) {
        foreach(const QString &targetName, wiz->relevantTargets()) {
            QCheckBox *check = new QCheckBox(targetName);
            check->setChecked(true);
            layout()->addWidget(check);
            connect(check,SIGNAL(stateChanged(int)),this,SIGNAL(completeChanged()));

            registerField(QStringLiteral("addTarget_")+targetName,check);
        }
    }
}

bool UbuntuSelectSubProjectsPage::isComplete() const
{
    //check if at least one item is selected
    UbuntuProjectMigrationWizard *wiz = qobject_cast<UbuntuProjectMigrationWizard *>(wizard());
    if(wiz) {
        foreach(const QString &targetName, wiz->relevantTargets()) {
            if(field(QStringLiteral("addTarget_")+targetName).toBool())
                return true;
        }
    }
    return false;
}

UbuntuProjectDetailsPage::UbuntuProjectDetailsPage(QWidget *parent) :
    QWizardPage(parent),
    m_initialized(false)
{

    QFormLayout *layout = new QFormLayout;

    QLineEdit *domainLineEdit = new QLineEdit;
    layout->addRow(tr("Domain"),domainLineEdit);
    registerField(QStringLiteral("domain*"),domainLineEdit);

    QLineEdit *maintainerLineEdit = new QLineEdit;
    layout->addRow(tr("Maintainer"),maintainerLineEdit);
    registerField(QStringLiteral("maintainer*"),maintainerLineEdit);

    QComboBox *box = new QComboBox;
    QStringList allFrameworks = UbuntuClickFrameworkProvider::getSupportedFrameworks();

    int running    = -1;
    int defaultIdx = -1;
    foreach(const QString &fw, allFrameworks) {
        if(defaultIdx == -1) {
            running++;
            if(!fw.contains(QStringLiteral("-dev")))
                defaultIdx = running;
        }

        box->addItem(fw,fw);
    }

    if(defaultIdx >= 0)
        box->setCurrentIndex(defaultIdx);

    layout->addRow(tr("Framework"),box);
    registerField(QStringLiteral("framework"),box,"currentText",SIGNAL(currentTextChanged(QString)));
    connect(box,SIGNAL(currentTextChanged(QString)),this,SIGNAL(completeChanged()));

    setLayout(layout);
}

void UbuntuProjectDetailsPage::initializePage()
{
    QWizardPage::initializePage();

    if(!m_initialized) {
        m_initialized = true;

        QString maintainer = QStringLiteral("username");
        QString whoami     = QStringLiteral("maintainerName");
        UbuntuBzr *bzr = UbuntuBzr::instance();

        if(!bzr->isInitialized()) {
            bzr->initialize();
            bzr->waitForFinished();
        }

        if(bzr->isInitialized()) {
            maintainer = bzr->launchpadId();
            whoami     = bzr->whoami();
        }

        wizard()->setField(QStringLiteral("domain"),QString(QStringLiteral("com.ubuntu.developer.")+maintainer));
        wizard()->setField(QStringLiteral("maintainer"),whoami);
    }

}

bool UbuntuProjectDetailsPage::isComplete() const
{
    return QWizardPage::isComplete();
}

} // namespace Internal
} // namespace Ubuntu
