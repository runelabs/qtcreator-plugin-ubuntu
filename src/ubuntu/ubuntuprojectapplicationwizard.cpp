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

#include "ubuntushared.h"
#include "ubuntuprojectapplicationwizard.h"
#include "ubuntuconstants.h"
#include "ubuntuproject.h"
#include <coreplugin/modemanager.h>
#include <app/app_version.h>
#include <projectexplorer/customwizard/customwizard.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <qtsupport/qtsupportconstants.h>
#include <coreplugin/icore.h>
#include <utils/filesearch.h>  // Utils::matchCaseReplacement
#include <qmlprojectmanager/qmlprojectmanager.h>
#include <qmakeprojectmanager/qmakeprojectmanager.h>
#include <cmakeprojectmanager/cmakekitinformation.h>
#include <cmakeprojectmanager/cmakeprojectmanager.h>
#include <cmakeprojectmanager/cmakeproject.h>
#include <qtsupport/qtkitinformation.h>
#include <QtGlobal>

#include <QIcon>
#include <QDir>
#include <QDebug>
#include <QTextStream>
#include <QDeclarativeEngine>
#include <QJsonArray>
#include <QDebug>

using namespace Ubuntu::Internal;

enum {
    debug = 0
};

UbuntuProjectApplicationWizardDialog::UbuntuProjectApplicationWizardDialog(QWidget *parent,
                                                                           const Core::WizardDialogParameters &parameters, const QString &projectType)
    : ProjectExplorer::BaseProjectWizardDialog(parent, parameters)
    , m_targetSetupPage(0)
    , m_projectType(projectType)
{
    setWindowTitle(tr("New Ubuntu QML Project"));
    setIntroDescription(tr("This wizard generates a Ubuntu QML project based on Ubuntu Components."));
    connect(this,SIGNAL(projectParametersChanged(QString,QString)),this,SLOT(on_projectParametersChanged(QString,QString)));
}

int UbuntuProjectApplicationWizardDialog::addTargetSetupPage(int id)
{
    m_targetSetupPage = new ProjectExplorer::TargetSetupPage;
    const QString platform = selectedPlatform();

    //prefer Qt Desktop or Platform Kit
    Core::FeatureSet features = Core::FeatureSet(QtSupport::Constants::FEATURE_DESKTOP);
    if (platform.isEmpty())
        m_targetSetupPage->setPreferredKitMatcher(new QtSupport::QtVersionKitMatcher(features));
    else
        m_targetSetupPage->setPreferredKitMatcher(new QtSupport::QtPlatformKitMatcher(platform));

    if(m_projectType == QLatin1String(Constants::UBUNTU_CMAKEPROJECT_TYPE)) {
        //make sure only CMake compatible Kits are shown
        m_targetSetupPage->setRequiredKitMatcher(new CMakeProjectManager::CMakeKitMatcher());
    } else {
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
            //this is just a fallback for now to remove all ubuntu kits until cross compiling is sorted out
            //it should not be hit at all but i keep it there just to be safe
            m_targetSetupPage->setRequiredKitMatcher(new QtSupport::QtVersionKitMatcher(features));
    }

    resize(900, 450);
    if (id >= 0)
        setPage(id, m_targetSetupPage);
    else
        id = addPage(m_targetSetupPage);

    wizardProgress()->item(id)->setTitle(tr("Kits"));
    return id;
}

bool UbuntuProjectApplicationWizardDialog::writeUserFile(const QString &cmakeFileName) const
{
    if (!m_targetSetupPage)
        return false;

    ProjectExplorer::IProjectManager *manager = 0;
    if(m_projectType == QLatin1String(Constants::UBUNTU_GOPROJECT_TYPE)) {
        manager = qobject_cast<ProjectExplorer::IProjectManager *>(ExtensionSystem::PluginManager::getObjectByClassName(QStringLiteral("GoLang::Internal::Manager")));
    } else {
        manager = ExtensionSystem::PluginManager::getObject<CMakeProjectManager::CMakeManager>();
        Q_ASSERT(manager);
    }

    if(!manager)
        return false;

    QString error;
    ProjectExplorer::Project* pro = manager->openProject(cmakeFileName,&error);
    if(pro) {
        if(debug)
            qDebug()<<"Storing project type: "<<pro->id().toSetting();
        bool success = m_targetSetupPage->setupProject(pro);
        if(success) {
            pro->saveSettings();
        }
        delete pro;
        return success;
    }

    return false;
}

void UbuntuProjectApplicationWizardDialog::on_projectParametersChanged(const QString &projectName, const QString &path)
{
    if(m_targetSetupPage) {
        if(m_projectType == QLatin1String(Constants::UBUNTU_GOPROJECT_TYPE)) {
            m_targetSetupPage->setProjectPath(path+QDir::separator()
                                              +projectName
                                              +QDir::separator()
                                              +QString::fromLatin1("%1.goproject").arg(projectName));
        } else {
            m_targetSetupPage->setProjectPath(path+QDir::separator()+projectName+QDir::separator()+QLatin1String("CMakeList.txt"));
        }
    }
}

UbuntuProjectApplicationWizard::UbuntuProjectApplicationWizard(QJsonObject obj)
    : Core::BaseFileWizard(),
      m_app(new UbuntuProjectApp())
{
    m_app->setupParameters(obj,this);
    m_app->setData(obj);

    m_projectType = obj.value(QLatin1String(Constants::UBUNTU_PROJECTJSON_TYPE)).toString();
}

UbuntuProjectApplicationWizard::~UbuntuProjectApplicationWizard()
{ }

Core::FeatureSet UbuntuProjectApplicationWizard::requiredFeatures() const
{
#ifdef Q_PROCESSOR_ARM
    return Core::Feature(m_app->requiredFeature());
#else
    return Core::Feature(QtSupport::Constants::FEATURE_QMLPROJECT)
            | Core::Feature(QtSupport::Constants::FEATURE_QT_QUICK_2)
            | Core::Feature(m_app->requiredFeature());
#endif
}

QWizard *UbuntuProjectApplicationWizard::createWizardDialog(QWidget *parent,
                                                            const Core::WizardDialogParameters &wizardDialogParameters) const
{
    UbuntuProjectApplicationWizardDialog *wizard = new UbuntuProjectApplicationWizardDialog(parent, wizardDialogParameters,m_projectType);

    if(m_projectType == QLatin1String(Constants::UBUNTU_CMAKEPROJECT_TYPE) || m_projectType == QLatin1String(Constants::UBUNTU_GOPROJECT_TYPE)) {
        wizard->addTargetSetupPage(1);
    }
    wizard->setProjectName(UbuntuProjectApplicationWizardDialog::uniqueProjectName(wizardDialogParameters.defaultPath()));
    wizard->addExtensionPages(wizardDialogParameters.extensionPages());
    return wizard;
}


Core::GeneratedFiles UbuntuProjectApplicationWizard::generateFiles(const QWizard *w,
                                                                   QString *errorMessage) const
{
    return m_app->generateFiles(w,errorMessage);
}

bool UbuntuProjectApplicationWizard::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage)
{
    const UbuntuProjectApplicationWizardDialog *dialog = qobject_cast<const UbuntuProjectApplicationWizardDialog *>(w);
    bool retval = true;
    // make sure that the project gets configured properly
    if (m_app->projectType() == QLatin1String(Constants::UBUNTU_QTPROJECT_TYPE)) {
        QmakeProjectManager::QmakeManager *manager = ExtensionSystem::PluginManager::getObject<QmakeProjectManager::QmakeManager>();
        ProjectExplorer::Project* project = new QmakeProjectManager::QmakeProject(manager, m_app->projectFileName());
        retval = BaseFileWizard::postGenerateOpenEditors(l,errorMessage);
        ProjectExplorer::ProjectExplorerPlugin::instance()->openProject(m_app->projectFileName(), errorMessage);
        if (project->needsConfiguration()) {
            Core::ModeManager::activateMode(ProjectExplorer::Constants::MODE_SESSION);
        }
        delete project;
    } else {
        if(m_app->projectType() == QLatin1String(Constants::UBUNTU_CMAKEPROJECT_TYPE)
                || m_app->projectType() == QLatin1String(Constants::UBUNTU_GOPROJECT_TYPE)) {
            // Generate user settings
            foreach (const Core::GeneratedFile &file, l)
                if (file.attributes() & Core::GeneratedFile::OpenProjectAttribute) {
                    dialog->writeUserFile(file.path());
                    break;
                }
        }

        retval = ProjectExplorer::CustomProjectWizard::postGenerateOpen(l,errorMessage);
    }

    return retval;
}

/**** statics ****/
QString UbuntuProjectApplicationWizard::templatesPath(QString fileName) {
    return Constants::UBUNTU_TEMPLATESPATH + fileName;
}

QByteArray UbuntuProjectApplicationWizard::getProjectTypesJSON() {
    QByteArray contents;
    QString errorMsg;
    if (readFile(templatesPath(QLatin1String(Constants::UBUNTU_PROJECTJSON)),&contents, &errorMsg) == false) {
        contents = errorMsg.toAscii();
        qDebug() << __PRETTY_FUNCTION__ << contents;
    }
    return contents;
}

