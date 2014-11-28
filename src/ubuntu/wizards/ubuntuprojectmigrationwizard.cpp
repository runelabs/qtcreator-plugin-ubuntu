#include "ubuntuprojectmigrationwizard.h"

#include "../ubuntuclicktool.h"
#include "../ubuntubzr.h"
#include "../ubuntuscopefinalizer.h"

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

namespace Ubuntu {
namespace Internal {


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

        QList<QmakeProjectManager::QmakeProFileNode *> nodes = project->allProFiles();
        foreach(QmakeProjectManager::QmakeProFileNode * node, nodes) {

            QtSupport::ProFileReader *reader = project->createProFileReader(node);

            OnScopeExit {
                qDebug()<<"Destroying reader";
                project->destroyProFileReader(reader);
            };

            bool canRead = true;
            //setup the file reader correctly
            if (ProFile *pro = reader->parsedProFile(project->projectFilePath())) {
                if(!reader->accept(pro, QMakeEvaluator::LoadAll)) {
                    canRead = false;
                }
                pro->deref();
            } else {
                canRead = false;
            }

            if(canRead) {
                QString targetInstallPath;

                node->setProVariable(QStringLiteral("qt_install_libs"),QStringList()<<QStringLiteral("$$[QT_INSTALL_LIBS]"));

                const auto projectType = node->projectType();

                QMap<QString,QString> mapPaths;

                //inspect installs
                QStringList installs = reader->values(QStringLiteral("INSTALLS"));
                if(installs.contains(QStringLiteral("target"))) {
                    installs.removeAll(QStringLiteral("target"));

                    // try to map to the click way
                    const QString path = reader->value(QStringLiteral("target.path"));

                    // just remove /usr if possible, except for ApplicationTemplates, because those can not be in
                    // /bin, but have to recide in /lib/gnu-triplet/bin
                    if(path.startsWith(QStringLiteral("/usr")) && QmakeProjectManager::ApplicationTemplate != projectType) {
                        targetInstallPath = path;
                        targetInstallPath.remove(0,4);
                    } else {
                        bool isLikelyQmlPlugin = false;
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

                        if(!isLikelyQmlPlugin) {
                            if(projectType == QmakeProjectManager::LibraryTemplate) {
                                targetInstallPath = QStringLiteral("/lib/$$basename(qt_install_libs)");
                            } else if(projectType == QmakeProjectManager::ApplicationTemplate) {
                                targetInstallPath = QStringLiteral("/lib/$$basename(qt_install_libs)/bin");
                            }
                        }
                    }

                    //make sure all files targeted to that path go into the same direction
                    mapPaths.insert(path,targetInstallPath);
                    node->setProVariable(QStringLiteral("target.path"),QStringList()<<targetInstallPath);
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

                                mappedPath = mapPaths[key] + QStringLiteral("/")+suffix;
                                mapPaths.insert(path,mappedPath);

                                found = true;
                                break;
                            }
                        }

                        if(!found) {
                            mappedPath.remove(QStringLiteral("/usr"));
                        }
                    }

                    node->setProVariable(install+QStringLiteral(".path"),QStringList()<<mappedPath);
                }

                //now add required files
                if(projectType == QmakeProjectManager::ApplicationTemplate) {
                    if(hookTargets.contains(node->targetInformation().target)) {
                        if(!multiTargetProject) {
                            //add manifest.pri
                        }
                       //add desktop and apparmor files
                    }
                    //handledTargetPath = true;
                }
            }
        }

        if(multiTargetProject) {
            //add manifest project
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
