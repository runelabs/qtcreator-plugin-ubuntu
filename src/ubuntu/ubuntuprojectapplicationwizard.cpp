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
#include <QtGlobal>

#include <QIcon>
#include <QDir>
#include <QDebug>
#include <QTextStream>
#include <QDeclarativeEngine>
#include <QJsonArray>

using namespace Ubuntu::Internal;

UbuntuProjectApplicationWizardDialog::UbuntuProjectApplicationWizardDialog(QWidget *parent,
                                                                     const Core::WizardDialogParameters &parameters) :
    ProjectExplorer::BaseProjectWizardDialog(parent, parameters)
{
    setWindowTitle(tr("New Ubuntu QML Project"));
    setIntroDescription(tr("This wizard generates a Ubuntu QML project based on Ubuntu Components."));
}

UbuntuProjectApplicationWizard::UbuntuProjectApplicationWizard(QJsonObject obj)
    : Core::BaseFileWizard(parameters(obj)),
      m_app(new UbuntuProjectApp())
{
    m_app->setData(obj);
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

Core::BaseFileWizardParameters UbuntuProjectApplicationWizard::parameters(QJsonObject params)
{
    return m_app->parameters(params);
}


QWizard *UbuntuProjectApplicationWizard::createWizardDialog(QWidget *parent,
                                                         const Core::WizardDialogParameters &wizardDialogParameters) const
{
    UbuntuProjectApplicationWizardDialog *wizard = new UbuntuProjectApplicationWizardDialog(parent, wizardDialogParameters);
    wizard->setProjectName(UbuntuProjectApplicationWizardDialog::uniqueProjectName(wizardDialogParameters.defaultPath()));
    wizard->addExtensionPages(wizardDialogParameters.extensionPages());
    return wizard;
}


Core::GeneratedFiles UbuntuProjectApplicationWizard::generateFiles(const QWizard *w,
                                                     QString *errorMessage) const
{
    return m_app->generateFiles(w,errorMessage);
}

bool UbuntuProjectApplicationWizard::postGenerateFiles(const QWizard *, const Core::GeneratedFiles &l, QString *errorMessage)
{

    bool retval = true;
    // make sure that the project gets configured properly
    if (m_app->projectType() == QLatin1String(Constants::UBUNTU_QTPROJECT_TYPE)) {
        Qt4ProjectManager::Qt4Manager *manager = ExtensionSystem::PluginManager::getObject<Qt4ProjectManager::Qt4Manager>();
        ProjectExplorer::Project* project = new Qt4ProjectManager::Qt4Project(manager, m_app->projectFileName());
        retval = BaseFileWizard::postGenerateOpenEditors(l,errorMessage);
        ProjectExplorer::ProjectExplorerPlugin::instance()->openProject(m_app->projectFileName(), errorMessage);
        if (project->needsConfiguration()) {
                Core::ModeManager::activateMode(ProjectExplorer::Constants::MODE_SESSION);
        }
        delete project;
    } else {
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

