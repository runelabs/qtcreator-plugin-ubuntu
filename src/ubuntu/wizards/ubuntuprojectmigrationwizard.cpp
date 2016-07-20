#include "ubuntuprojectmigrationwizard.h"

#include "../ubuntuclicktool.h"
#include "../ubuntubzr.h"
#include "../ubuntuscopefinalizer.h"
#include "../ubuntuconstants.h"
#include "../ubuntushared.h"

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
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QCoreApplication>

namespace Ubuntu {
namespace Internal {

static bool createFileFromTemplate (const QString &templateFile, const QString &targetFile, const QMap<QString,QString> replacements)
{
    QString templateText;
    QFile inFile(templateFile);
    if(Q_UNLIKELY(!inFile.open(QIODevice::ReadOnly))) {
        printToOutputPane(QCoreApplication::translate("UbuntuProjectMigrationWizard","Could not open template file %1. The project will miss this file.")
                          .arg(templateFile));
        return false;
    }

    QFile outFile(targetFile);
    QFileInfo outFileInfo(targetFile);

    QDir d;
    if(Q_UNLIKELY(!d.mkpath(outFileInfo.absolutePath()))) {
        printToOutputPane(QCoreApplication::translate("UbuntuProjectMigrationWizard","Could not create the path %1. The project will miss files.")
                          .arg(outFileInfo.absolutePath()));
        return false;
    }

    if(!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        printToOutputPane(QCoreApplication::translate("UbuntuProjectMigrationWizard","Could not create %1. The project will miss files.")
                          .arg(targetFile));
        return false;
    }

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
    const int introPageId        = addPage(new UbuntuProjectMigrationIntroPage);
    const int selectTargetPageId = addPage(new UbuntuSelectSubProjectsPage);
    const int projectsInfoPageId = addPage(new UbuntuProjectDetailsPage);

    Utils::WizardProgress *progress = wizardProgress();

    progress->item(introPageId)->setTitle(tr("Intro"));
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
        bool multiTargetProject = project->rootProjectNode()->projectType() == QmakeProjectManager::SubDirsTemplate;


        QMap<QString,QString> base_replacements;
        base_replacements.insert(QStringLiteral("ProjectName"),project->displayName());
        base_replacements.insert(QStringLiteral("ClickDomain"),wiz.domain());
        base_replacements.insert(QStringLiteral("ClickMaintainer"),wiz.maintainer());

        QString framework = wiz.framework();
        QString aaPolicy  = UbuntuClickFrameworkProvider::instance()->frameworkPolicy(framework);
        base_replacements.insert(QStringLiteral("ClickFrameworkVersion"),framework);
        base_replacements.insert(QStringLiteral("ClickAAPolicyVersion"),aaPolicy.isEmpty() ? QStringLiteral("1.3") : aaPolicy);

        QStringList hookTargets = wiz.selectedTargets();

        //contains all old paths mapped to the path they should have inthe click package
        QMap<QString,QString> mapPaths;


        QList<QmakeProjectManager::QmakeProFileNode *> nodes = project->allProFiles();
        foreach(QmakeProjectManager::QmakeProFileNode * node, nodes) {

            if(node->projectType() == QmakeProjectManager::SubDirsTemplate)
                continue;

            QtSupport::ProFileReader *reader = project->createProFileReader(node);
            OnScopeExit {
                project->destroyProFileReader(reader);
            };

            bool canRead = true;
            //setup the file reader correctly

            if (ProFile *pro = reader->parsedProFile(node->filePath().toString())) {
                if(!reader->accept(pro, QMakeEvaluator::LoadAll)) {
                    canRead = false;
                }
                pro->deref();
            } else {
                canRead = false;
            }


            if(!canRead) {
                printToOutputPane(tr("Can not parse %1, skipping migration.").arg(node->filePath().toString()));
                continue;
            }

            QMap<QString,QStringList> variablesToSetAtEnd;
            auto setVarAtEnd = [&] (const QString &name, const QStringList &list) {
                if(variablesToSetAtEnd.contains(name))
                    variablesToSetAtEnd[name].append(list);
                else
                    variablesToSetAtEnd.insert(name,list);
            };

            QString targetInstallPath;
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

                if(projectType == QmakeProjectManager::SharedLibraryTemplate) {
                    //this is probably a qml plugin, need to analyze more
                    foreach(const QString &install, installs) {
                        if(install == QStringLiteral("target"))
                            continue;

                        bool found = false;
                        QStringList files = reader->values(install+QStringLiteral(".files"));
                        foreach(const QString &file,files) {
                            QFileInfo info(file);
                            if(info.completeBaseName() == QStringLiteral("qmldir")) {
                                //-> we have a qml plugin
                                //use the last directory of the path

                                isLikelyQmlPlugin = true;

                                QString inst_path = reader->value(install+QStringLiteral(".path"));
                                QString suffix = inst_path.split(QStringLiteral("/")).last();

                                targetInstallPath = QStringLiteral("/lib/$$basename(qt_install_libs)/qt5/qml/")+suffix;

                                //make sure this install targets contents go there as well
                                mapPaths.insert(QDir::cleanPath(inst_path),QDir::cleanPath(targetInstallPath));

                                found = true;
                                break;
                            }
                        }

                        if(found)
                            break;
                    }
                }

                //make sure all files targeted to that path go into the same direction
                mapPaths.insert(QDir::cleanPath(path),targetInstallPath);
            }

            if(!hasInstallTarget || !isLikelyQmlPlugin) {
                if(projectType == QmakeProjectManager::SharedLibraryTemplate) {
                    targetInstallPath = QStringLiteral("/lib/$$basename(qt_install_libs)");
                } else if(projectType == QmakeProjectManager::ApplicationTemplate) {
                    targetInstallPath = QStringLiteral("/lib/$$basename(qt_install_libs)/bin");
                }
            }

            //BEGIN WRITE
            node->setProVariable(QStringLiteral("CONFIG"),
                                 QStringList()<<QStringLiteral("ubuntu-click"),
                                 QString(),
                                 QmakeProjectManager::Internal::ProWriter::AppendValues
                                 | QmakeProjectManager::Internal::ProWriter::AppendOperator);

            node->setProVariable(QStringLiteral("qt_install_libs"),
                                 QStringList()<<QStringLiteral("$$[QT_INSTALL_LIBS]"),
                                 QStringLiteral("ubuntu-click"));

            node->setProVariable(QStringLiteral("target.path"),
                                 QStringList()<<targetInstallPath,
                                 QStringLiteral("ubuntu-click"));

            if(!hasInstallTarget)
                setVarAtEnd(QStringLiteral("INSTALLS"),QStringList()<<QStringLiteral("target"));


            //now fix all the other installs
            foreach(const QString &install, installs) {
                const QString path = QDir::cleanPath(reader->value(install+QStringLiteral(".path")));
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

                QFileInfo proFilePath(node->filePath().toFileInfo());

                QmakeProjectManager::TargetInformation targetInfo = node->targetInformation();
                if(hookTargets.contains(targetInfo.target)) {
                    QMap<QString,QString> replacements = base_replacements;
                    replacements.insert(QStringLiteral("ClickHookName"),targetInfo.target);

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


                        node->setProVariable(QStringLiteral("manifest_install.path"),
                                             QStringList() << QStringLiteral("/"),
                                             QStringLiteral("ubuntu-click"));

                        node->setProVariable(QStringLiteral("manifest_install.files"),
                                             QStringList()<<QStringLiteral("manifest.json"),
                                             QStringLiteral("ubuntu-click"));

                        node->addFiles(QStringList()<<QStringLiteral("manifest.json.in"));

                        setVarAtEnd(QStringLiteral("QMAKE_EXTRA_TARGETS"),QStringList()<<QStringLiteral("manifest"));
                        setVarAtEnd(QStringLiteral("PRE_TARGETDEPS"),QStringList()<<QStringLiteral("$$manifest.target"));
                        setVarAtEnd(QStringLiteral("INSTALLS"),QStringList()<<QStringLiteral("manifest_install"));
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
                                         QmakeProjectManager::Internal::ProWriter::AppendValues
                                         | QmakeProjectManager::Internal::ProWriter::MultiLine);

                    setVarAtEnd(QStringLiteral("INSTALLS"),QStringList()<<QStringLiteral("appDeps"));
                }
            }

            for(auto i = variablesToSetAtEnd.constBegin(); i != variablesToSetAtEnd.constEnd(); i++) {
                node->setProVariable(i.key(),
                                     i.value(),
                                     QStringLiteral("ubuntu-click"),
                                     QmakeProjectManager::Internal::ProWriter::AppendValues
                                     | QmakeProjectManager::Internal::ProWriter::AppendOperator
                                     | QmakeProjectManager::Internal::ProWriter::MultiLine);
            }
        }

        if(multiTargetProject) {

            QString manifestFilePath = QString::fromLatin1("%1/%2").arg(project->projectDirectory().toString()).arg(QStringLiteral("/manifest.json.in"));

            //add manifest pro file
            createFileFromTemplate(
                        QString::fromLatin1("%1/templates/wizards/ubuntu/bin_app-qmake/manifest/manifest.pro").arg(Constants::UBUNTU_RESOURCE_PATH),
                        QString::fromLatin1("%1/%2").arg(project->projectDirectory().toString()).arg(QStringLiteral("/manifest.pro")),
                        base_replacements
                        );
            createFileFromTemplate(
                        QString::fromLatin1("%1/templates/wizards/ubuntu/bin_app-qmake/manifest/manifest.json.in").arg(Constants::UBUNTU_RESOURCE_PATH),
                        manifestFilePath,
                        base_replacements
                        );

            //replacements.insert(QStringLiteral("ClickHookName"),targetInfo.target);
            //now we need to add all the selected hooks
            QFile manifestFile(manifestFilePath);
            if(Q_UNLIKELY(!manifestFile.open(QIODevice::ReadOnly))) {
                printToOutputPane(tr("Can not open manifest file for reading. Hooks need to be added manually."));
                return;
            }

            QJsonDocument doc = QJsonDocument::fromJson(manifestFile.readAll());
            QJsonObject rootObj = doc.object();

            manifestFile.close();

            QJsonObject hooksObj;
            foreach (const QString &hookName, hookTargets) {
                QJsonObject hook;
                hook.insert(QStringLiteral("apparmor"),QString::fromLatin1("%1/%2.apparmor").arg(hookName).arg(hookName));
                hook.insert(QStringLiteral("desktop"),QString::fromLatin1("%1/%2.desktop").arg(hookName).arg(hookName));
                hooksObj.insert(hookName,hook);
            }

            rootObj[QStringLiteral("hooks")] = hooksObj;

            doc.setObject(rootObj);

            if(!manifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate)){
                printToOutputPane(tr("Can not open manifest file for writing. Hooks need to be added manually."));
                return;
            }

            manifestFile.write(doc.toJson());
            manifestFile.close();

            project->rootProjectNode()->addSubProjects(QStringList()<<QString::fromLatin1("%1/%2").arg(project->projectDirectory().toString()).arg(QStringLiteral("/manifest.pro")));
        }
    }
}


UbuntuProjectMigrationIntroPage::UbuntuProjectMigrationIntroPage(QWidget *parent) : QWizardPage(parent)
{
    QLabel *label = new QLabel(tr("<h2 style=\"text-align: center;\">Ubuntu Project migration wizard</h2>"
                                  "<p>This wizard rewrites a qmake based project to generate a click package compatible install target.<br>"
                                  "Where possible, changes will be put into the <i>ubuntu-click</i> scope."
                                  "</p>"
                                  "<p>"
                                  "Please make sure your project adds all required files for your project to the INSTALL target."
                                  "</p>"
                                  "<p></p>"
                                  "<p>"
                                  "<span style=\"font-weight:bold\">Note:</span> This wizard may produce a faulty configuration in case of complex projects.<br>"
                                  "<span style=\"font-weight:bold;color:red;\">Warning:</span> Please make sure to backup your project before running this wizard!"
                                  "</p>"
                                  ));
    label->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    setLayout(layout);
}

UbuntuSelectSubProjectsPage::UbuntuSelectSubProjectsPage(QWidget *parent) : QWizardPage(parent)
{
    QLabel *label = new QLabel(tr("<p style=\"text-align: center;\">Please select the applications you want to add to the manifest file</p>"));
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

        wizard()->setField(QStringLiteral("domain"),maintainer);
        wizard()->setField(QStringLiteral("maintainer"),whoami);
    }

}

bool UbuntuProjectDetailsPage::isComplete() const
{
    return QWizardPage::isComplete();
}

} // namespace Internal
} // namespace Ubuntu
