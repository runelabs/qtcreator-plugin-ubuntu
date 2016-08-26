/*
 * Copyright 2013 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.1.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Juhapekka Piiroinen <juhapekka.piiroinen@canonical.com>
 */

#include "ubuntuprojectapplicationwizard.h"
#include "ubuntufirstrunwizard.h"
#include "../ubuntushared.h"
#include "../ubuntuconstants.h"
#include "../ubuntuproject.h"
#include "../ubuntubzr.h"
#include "../ubuntuclicktool.h"

#include <utils/qtcassert.h>
#include <utils/pathchooser.h>
#include <utils/mimetypes/mimedatabase.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <projectexplorer/kitmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <cmakeprojectmanager/cmakekitinformation.h>
#include <cmakeprojectmanager/cmaketool.h>
#include <coreplugin/id.h>

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/customwizard/customwizardparameters.h>

#include <QDir>
#include <QComboBox>


using namespace Ubuntu::Internal;

enum {
    debug = 0
};


UbuntuProjectApplicationWizard::UbuntuProjectApplicationWizard(ProjectType type)
    : m_type(type)
{
    setRequiredFeatures(requiredFeatures());
}

Core::BaseFileWizard *UbuntuProjectApplicationWizard::create(QWidget *parent, const Core::WizardDialogParameters &wizardDialogParameters) const
{
    QTC_ASSERT(!parameters().isNull(), return 0);
    UbuntuProjectApplicationWizardDialog *projectDialog = new UbuntuProjectApplicationWizardDialog(this,
                                                                                                   parent,
                                                                                                   m_type ,
                                                                                                   wizardDialogParameters);
    projectDialog->addChrootSetupPage(12);
    projectDialog->addTargetSetupPage(13);

    initProjectWizardDialog(projectDialog,
                            wizardDialogParameters.defaultPath(),
                            projectDialog->extensionPages());

    QString maintainer = QStringLiteral("username");
    QString whoami     = QStringLiteral("Firstname Surname <your@mail.com>");
    UbuntuBzr *bzr = UbuntuBzr::instance();

    if(!bzr->isInitialized()) {
        bzr->initialize();
        bzr->waitForFinished();
    }

    if(bzr->isInitialized()) {
        maintainer = bzr->launchpadId();
        whoami     = bzr->whoami();
    }

    projectDialog->setField(QStringLiteral("ClickMaintainer"),whoami);
    projectDialog->setField(QStringLiteral("ClickDomain"),maintainer);

    QList<QComboBox*> boxes = projectDialog->findChildren<QComboBox*>();
    foreach(QComboBox* box, boxes){
        if(box->currentData().toString() == QStringLiteral("ubuntu-sdk-dummy-framework")) {
            QStringList allFrameworks = UbuntuClickFrameworkProvider::getSupportedFrameworks();
            box->clear();

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

            break;
        }
    }


    return projectDialog;
}

bool UbuntuProjectApplicationWizard::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage) const
{
    const UbuntuProjectApplicationWizardDialog *dialog = qobject_cast<const UbuntuProjectApplicationWizardDialog *>(w);

    // Generate user settings
    foreach (const Core::GeneratedFile &file, l)
        if (file.attributes() & Core::GeneratedFile::OpenProjectAttribute) {
            dialog->writeUserFile(file.path());
            break;
        }

    // Post-Generate: Open the projects/editors
    return ProjectExplorer::CustomProjectWizard::postGenerateOpen(l ,errorMessage);
}

Core::GeneratedFiles UbuntuProjectApplicationWizard::generateFiles(const QWizard *w, QString *errorMessage) const
{
    QString requiredPolicy = UbuntuClickFrameworkProvider::instance()->frameworkPolicy(w->field(QStringLiteral("ClickFrameworkVersion")).toString());
    if(requiredPolicy.isEmpty())
        requiredPolicy = QStringLiteral("1.3"); //some sane default value

    context()->baseReplacements.insert(QStringLiteral("ClickAAPolicyVersion"),requiredPolicy);
    return ProjectExplorer::CustomProjectWizard::generateFiles(w,errorMessage);
}


QSet<Core::Id> UbuntuProjectApplicationWizard::requiredFeatures() const
{
#ifdef Q_PROCESSOR_ARM
    return CustomProjectWizard::requiredFeatures();
#else
    QSet<Core::Id> features = CustomProjectWizard::requiredFeatures();
    features << QtSupport::Constants::FEATURE_QMLPROJECT
             << Core::Id::versionedId(QtSupport::Constants::FEATURE_QT_QUICK_PREFIX, 2);

    return features;
#endif
}

UbuntuProjectApplicationWizardDialog::UbuntuProjectApplicationWizardDialog(const Core::BaseFileWizardFactory *factory, QWidget *parent,
                                                                           UbuntuProjectApplicationWizard::ProjectType type,
                                                                           const Core::WizardDialogParameters &parameters)
    : ProjectExplorer::BaseProjectWizardDialog(factory, parent, parameters)
    , m_targetSetupPage(0)
    , m_type(type)
{
    init();
}

UbuntuProjectApplicationWizardDialog::~UbuntuProjectApplicationWizardDialog()
{
    if (m_targetSetupPage && !m_targetSetupPage->parent())
        delete m_targetSetupPage;
}

bool UbuntuProjectApplicationWizardDialog::writeUserFile(const QString &projectFileName) const
{
    if (!m_targetSetupPage)
        return false;

    QFileInfo fi = QFileInfo(projectFileName);
    if (!fi.exists())
        return false;

    QString filePath = fi.canonicalFilePath();

    Utils::MimeDatabase mimeDb;
    const Utils::MimeType mt = mimeDb.mimeTypeForFile(fi);
    if (mt.isValid()) {
        QList<ProjectExplorer::IProjectManager*> allProjectManagers = ExtensionSystem::PluginManager::getObjects<ProjectExplorer::IProjectManager>();
        foreach (ProjectExplorer::IProjectManager *manager, allProjectManagers) {
            if (manager->mimeType() == mt.name()) {
                QString tmp;
                if (ProjectExplorer::Project *pro = manager->openProject(filePath, &tmp)) {
                    if(debug) qDebug()<<"Storing project type settings: "<<pro->id().toSetting();

                    bool success = m_targetSetupPage->setupProject(pro);
                    if(success) {
                        pro->saveSettings();
                    }
                    delete pro;
                    return success;
                }
                break;
            }
        }
    }
    return false;
}


void UbuntuProjectApplicationWizardDialog::init()
{
    setWindowTitle(tr("New Ubuntu Project"));
    setIntroDescription(tr("This wizard generates a Ubuntu project based on Ubuntu Components."));

    connect(this, SIGNAL(projectParametersChanged(QString,QString)),
            this, SLOT(generateProfileName(QString,QString)));
}

void UbuntuProjectApplicationWizardDialog::addChrootSetupPage(int id)
{
    QList<ProjectExplorer::Kit *> allKits = ProjectExplorer::KitManager::kits();

    bool found = false;
    foreach(ProjectExplorer::Kit *curr, allKits) {
        ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(curr);
        if (tc->typeId() == Constants::UBUNTU_CLICK_TOOLCHAIN_ID) {
            found = true;
            break;
        }
    }

    if(found)
        return;

    if (id >= 0)
        setPage(id, new UbuntuSetupChrootWizardPage);
    else
        id = addPage(new UbuntuSetupChrootWizardPage);
}

void UbuntuProjectApplicationWizardDialog::addTargetSetupPage(int id)
{
    m_targetSetupPage = new ProjectExplorer::TargetSetupPage;

    //prefer Desktop
    auto desktopMatcher = [](const ProjectExplorer::Kit *k) {
        return ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(k)
                .toString().startsWith(QLatin1String(Constants::UBUNTU_CONTAINER_DEVICE_TYPE_ID));
    };
    m_targetSetupPage->setPreferredKitMatcher(ProjectExplorer::KitMatcher(desktopMatcher));

    switch (m_type) {
        case UbuntuProjectApplicationWizard::CMakeProject:{
            auto cmakeKitMatcher = [](const ProjectExplorer::Kit *k){
                auto tool = CMakeProjectManager::CMakeKitInformation::cmakeTool(k);
                if (!tool && tool->isValid())
                    return false;

                UbuntuKitMatcher m;
                return m.matches(k);
            };
            m_targetSetupPage->setRequiredKitMatcher(ProjectExplorer::KitMatcher(cmakeKitMatcher));
            break;
        }
        case UbuntuProjectApplicationWizard::GoProject: {
#if 0
            ProjectExplorer::KitMatcher *matcher = 0;
            const char* retTypeName = QMetaType::typeName(qMetaTypeId<void*>());
            void **arg = reinterpret_cast<void**>(&matcher);
            ProjectExplorer::IProjectManager *manager = qobject_cast<ProjectExplorer::IProjectManager *>(ExtensionSystem::PluginManager::getObjectByClassName(QStringLiteral("GoLang::Internal::Manager")));
            if (manager) {
                bool success = QMetaObject::invokeMethod(manager,
                                                         "createKitMatcher",
                                                         QGenericReturnArgument(retTypeName,arg));
                if(!success && debug)
                     qDebug()<<"Invoke failed";
            }

            if (matcher)
                m_targetSetupPage->setRequiredKitMatcher(matcher);
            else
#endif
                //this is just a fallback for now to remove all ubuntu kits until cross compiling is sorted out
                //it should not be hit at all but i keep it there just to be safe
                m_targetSetupPage->setRequiredKitMatcher(QtSupport::QtKitInformation::platformMatcher(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE));
            break;
        }
        case UbuntuProjectApplicationWizard::QMakeProject:
        case UbuntuProjectApplicationWizard::UbuntuQMLProject:
        case UbuntuProjectApplicationWizard::UbuntuHTMLProject: {
            m_targetSetupPage->setRequiredKitMatcher(UbuntuKitMatcher());
            break;
        }
        default:
            break;
    }
    resize(900, 450);
    if (id >= 0)
        setPage(id, m_targetSetupPage);
    else
        id = addPage(m_targetSetupPage);

    wizardProgress()->item(id)->setTitle(tr("Kits"));
}

QList<Core::Id> UbuntuProjectApplicationWizardDialog::selectedKits() const
{
    if(m_targetSetupPage)
        return m_targetSetupPage->selectedKits();

    return QList<Core::Id>();
}

void UbuntuProjectApplicationWizardDialog::generateProfileName(const QString &projectName, const QString &path)
{
    if(!m_targetSetupPage)
        return;

    switch(m_type) {
        case UbuntuProjectApplicationWizard::QMakeProject: {
            m_targetSetupPage->setProjectPath(path+QDir::separator()
                                              +projectName
                                              +QDir::separator()
                                              +QString::fromLatin1("%1.pro").arg(projectName));
            break;
        }
        case UbuntuProjectApplicationWizard::GoProject: {
            m_targetSetupPage->setProjectPath(path+QDir::separator()
                                              +projectName
                                              +QDir::separator()
                                              +QString::fromLatin1("%1.goproject").arg(projectName));
            break;
        }
        case UbuntuProjectApplicationWizard::UbuntuHTMLProject: {
            m_targetSetupPage->setProjectPath(path+QDir::separator()
                                              +projectName
                                              +QDir::separator()
                                              +QString::fromLatin1("%1.ubuntuhtmlproject").arg(projectName));
            break;
        }
        case UbuntuProjectApplicationWizard::UbuntuQMLProject: {
            m_targetSetupPage->setProjectPath(path+QDir::separator()
                                              +projectName
                                              +QDir::separator()
                                              +QString::fromLatin1("%1.qmlproject").arg(projectName));
            break;
        }
        default: {
            m_targetSetupPage->setProjectPath(path+QDir::separator()+projectName+QDir::separator()+QLatin1String("CMakeLists.txt"));
        }
    }
}
