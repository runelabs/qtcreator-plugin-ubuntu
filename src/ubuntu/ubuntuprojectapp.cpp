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

#include "ubuntuprojectapp.h"

#include "ubuntushared.h"
#include "ubuntuprojectapplicationwizard.h"
#include "ubuntuconstants.h"
#include "ubuntuproject.h"

#include <app/app_version.h>
#include <projectexplorer/customwizard/customwizard.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <qtsupport/qtsupportconstants.h>
#include <coreplugin/icore.h>
#include <utils/filesearch.h>
#include <qmlprojectmanager/qmlprojectmanager.h>
#include <qt4projectmanager/qt4projectmanager.h>
#include <QtGlobal>

#include <QIcon>
#include <QDir>
#include <QDebug>
#include <QTextStream>
#include <QDeclarativeEngine>
#include <QJsonArray>

using namespace Ubuntu;
using namespace Ubuntu::Internal;

UbuntuProjectApp::UbuntuProjectApp(QObject *parent) :
    QObject(parent)
{
    m_bzrInfo.initialize();
}

Core::GeneratedFiles UbuntuProjectApp::generateFiles(const QWizard *w, QString *errorMessage) {
    Q_UNUSED(errorMessage);

    QByteArray contents;
    Core::GeneratedFiles files;

    QJsonValue tmp_type = m_obj.value(QLatin1String(Constants::UBUNTU_PROJECTJSON_TYPE));
    QString projectType = QLatin1String(Constants::UBUNTU_QMLPROJECT_TYPE);
    if (tmp_type.isUndefined() == false) {
        projectType = tmp_type.toString();
    }

    setProjectType(projectType);

    QJsonValue tmp_hasTests = m_obj.value(QLatin1String(Constants::UBUNTU_HAS_TESTS));
    bool hasTests = false;
    if (tmp_hasTests.isUndefined() == false) {
        hasTests = tmp_hasTests.toBool();
    }

    const UbuntuProjectApplicationWizardDialog *wizard = qobject_cast<const UbuntuProjectApplicationWizardDialog *>(w);
    const QString projectName = wizard->projectName();
    const QString projectPath = wizard->path() + QLatin1Char('/') + projectName;


    QString folder;
    QJsonValue tmp_folder = m_obj.value(QLatin1String(Constants::UBUNTU_PROJECTJSON_FOLDER));
    if (tmp_folder.isUndefined() == false) {
        folder = tmp_folder.toString();
    }

    if (m_projectType == QLatin1String(Constants::UBUNTU_QMLPROJECT_TYPE)
            || m_projectType == QLatin1String(Constants::UBUNTU_AUTOPILOTPROJECT_TYPE)
            || m_projectType == QLatin1String(Constants::UBUNTU_HTMLPROJECT_TYPE)
            || m_projectType == QLatin1String(Constants::UBUNTU_UBUNTUPROJECT_TYPE)) {

        const QString creatorFileName = Core::BaseFileWizard::buildFileName(projectPath,
                                                                            projectName,
                                                                            m_projectType);
        QString mainFileName;

        // load the mainFile
        if (m_projectType == QLatin1String(Constants::UBUNTU_HTMLPROJECT_TYPE)) {
            mainFileName = Core::BaseFileWizard::buildFileName(projectPath + QLatin1String("/www"),
                                                              QLatin1String("index"),
                                                              QLatin1String("html"));
        } else {
            mainFileName = Core::BaseFileWizard::buildFileName(projectPath,
                                                               projectName,
                                                               QLatin1String(Constants::UBUNTU_QML_TYPE));
        }
        QJsonValue tmp_mainFile = m_obj.value(QLatin1String(Constants::UBUNTU_PROJECTJSON_MAINFILE));

        if (tmp_mainFile.isUndefined() == false && tmp_folder.isUndefined() == false) {
            QString errorMsg;
            QString fileName = tmp_mainFile.toString();

            if (readFile(QString(QLatin1String("%0/%1")).arg(UbuntuProjectApplicationWizard::templatesPath(folder)).arg(fileName),&contents, &errorMsg) == false) {
                contents = errorMsg.toAscii();
                qDebug() << __PRETTY_FUNCTION__ << contents;
            }
        }

        contents = processReservedWords(contents,projectPath,projectName);
        mainFileName = processReservedWordsInFileName(mainFileName,projectName);

        Core::GeneratedFile generatedMainFile(mainFileName);
        generatedMainFile.setContents(QString::fromLatin1(contents));
        generatedMainFile.setAttributes(Core::GeneratedFile::OpenEditorAttribute);

        // create the project file
        QByteArray projectContents;
        {
            QTextStream out(&projectContents);

            QDeclarativeEngine engine; // QQmlEngine engine;

            out << "/* File generated by Qt Creator (with Ubuntu Plugin), version " << Core::Constants::IDE_VERSION_LONG << " */" << endl
                << endl
                << "import QmlProject 1.1" << endl
                << endl
                << "Project {" << endl
                << "    mainFile: \"" << QDir(projectPath).relativeFilePath(mainFileName) << '"' << endl
                << endl
                << "    /* Include .qml, .js, and image files from current directory and subdirectories */" << endl
                << "    QmlFiles {" << endl
                << "        directory: \".\"" << endl
                << "    }" << endl
                << "    JavaScriptFiles {" << endl
                << "        directory: \".\"" << endl
                << "    }" << endl
                << "    ImageFiles {" << endl
                << "        directory: \".\"" << endl
                << "    }" << endl
                << "    Files {" << endl
                << "        filter: \"*.desktop\"" << endl
                << "    }" << endl
                << "    Files {" << endl
                << "        filter: \"www/*.html\"" << endl
                << "    }" << endl
                << "    Files {" << endl
                << "        filter: \"Makefile\"" << endl
                << "    }" << endl;

                out << "    Files {" << endl
                    << "        directory: \"www\"" << endl
                    << "        filter: \"*\"" << endl
                    << "    }" << endl;

                out << "    Files {" << endl
                    << "        directory: \"www/img/\"" << endl
                    << "        filter: \"*\"" << endl
                    << "    }" << endl;

                out << "    Files {" << endl
                    << "        directory: \"www/css/\"" << endl
                    << "        filter: \"*\"" << endl
                    << "    }" << endl;

                out << "    Files {" << endl
                    << "        directory: \"www/js/\"" << endl
                    << "        filter: \"*\"" << endl
                    << "    }" << endl;

                out << "    Files {" << endl
                    << "        directory: \"tests/\"" << endl
                    << "        filter: \"*\"" << endl
                    << "    }" << endl;
           // }
            out << "    Files {" << endl
                << "        directory: \"debian\"" << endl
                << "        filter: \"*\"" << endl
                << "    }" << endl
                << "    /* List of plugin directories passed to QML runtime */" << endl
                << "    importPaths: [ \".\" ,\""

                // FIX ME: use QQmlEngine instead of QDeclarativeEngine. Then remove the replace.
                <<  engine.importPathList().join(QLatin1String("\",\"")).replace(QLatin1String("imports"),QLatin1String("qml"))

                << "\" ]" << endl

            << "}" << endl;
        }

        setProjectFileName(creatorFileName);
        Core::GeneratedFile generatedCreatorFile(creatorFileName);
        generatedCreatorFile.setContents(QLatin1String(projectContents));
        generatedCreatorFile.setAttributes(Core::GeneratedFile::OpenProjectAttribute);

        // add created files
        files.append(generatedMainFile);
        files.append(generatedCreatorFile);
    } else {

        const bool isCMakeProject = (m_projectType == QLatin1String(Constants::UBUNTU_CMAKEPROJECT_TYPE));

        QString errorMsg;
        QJsonValue tmp_mainFile = m_obj.value(QLatin1String(Constants::UBUNTU_PROJECTJSON_MAINFILE));
        if (tmp_mainFile.isUndefined() == false) {
            QString fileName_target = Core::BaseFileWizard::buildFileName(projectPath, tmp_mainFile.toString(),QString());
            QString fileName_source = tmp_mainFile.toString();
            if (readFile(QString(QLatin1String("%0/%1")).arg(UbuntuProjectApplicationWizard::templatesPath(folder)).arg(fileName_source),&contents, &errorMsg) == false) {
                contents = errorMsg.toAscii();
                qDebug() << __PRETTY_FUNCTION__ << contents;
            }

            contents = processReservedWords(contents,projectPath,projectName);

            fileName_target = processReservedWordsInFileName(fileName_target,projectName);


            Core::GeneratedFile generatedMainFile(fileName_target);
            generatedMainFile.setContents(QLatin1String(contents));
            generatedMainFile.setAttributes(Core::GeneratedFile::OpenEditorAttribute);
            files.append(generatedMainFile);
        }

        QJsonValue tmp_projectFile = m_obj.value(QLatin1String(Constants::UBUNTU_PROJECTJSON_PROJECTFILE));
        if (tmp_projectFile.isUndefined() == false) {

            QString fileName_target = isCMakeProject ? QString::fromLatin1("%0/CMakeLists.txt").arg(projectPath)
                                                     : Core::BaseFileWizard::buildFileName(projectPath, QString(QLatin1String("%0.%1")).arg(projectName).arg(QLatin1String(Constants::UBUNTU_QTPROJECT_TYPE)),QString());

            QString fileName_source = tmp_projectFile.toString();
            if (readFile(QString(QLatin1String("%0/%1")).arg(UbuntuProjectApplicationWizard::templatesPath(folder)).arg(fileName_source),&contents, &errorMsg) == false) {
                contents = errorMsg.toAscii();
                qDebug() << __PRETTY_FUNCTION__ << contents;
            }

            contents = processReservedWords(contents,projectPath,projectName);

            fileName_target = processReservedWordsInFileName(fileName_target,projectName);

            setProjectFileName(fileName_target);

            Core::GeneratedFile generatedCreatorFile(fileName_target);
            generatedCreatorFile.setContents(QLatin1String(contents));
            generatedCreatorFile.setAttributes(Core::GeneratedFile::OpenProjectAttribute);
            files.append(generatedCreatorFile);
        }
    }

    // create and add the other files
    QJsonValue tmp = m_obj.value(QLatin1String(Constants::UBUNTU_PROJECTJSON_FILES));
    if (tmp.isUndefined() == false && tmp.isArray() ) {
        QJsonArray jsonFiles = tmp.toArray();
        for (int idx = 0; idx < jsonFiles.size(); idx++) {
            QJsonValue tmp_value = jsonFiles.at(idx);
            if (tmp_value.isObject()) {
                QJsonValue tmp_fileName = tmp_value.toObject().value(QLatin1String(Constants::UBUNTU_PROJECTJSON_FILENAME));
                if (tmp_fileName.isUndefined() == false) {
                    QString fileName = tmp_fileName.toString();
                    QString errorMsg;
                    QByteArray contents;
                    if (readFile(QString(QLatin1String("%0/%1")).arg(UbuntuProjectApplicationWizard::templatesPath(folder)).arg(fileName),&contents, &errorMsg) == false) {
                        contents = errorMsg.toAscii();
                        qDebug() << __PRETTY_FUNCTION__ << contents;
                    } else {
                        contents = processReservedWords(contents,projectPath,projectName);

                        fileName = processReservedWordsInFileName(fileName,projectName);

                        Core::GeneratedFile generatedFile(Core::BaseFileWizard::buildFileName(projectPath,fileName,QString()));
                        generatedFile.setBinaryContents(contents);
                        files.append(generatedFile);
                    }
                }
            }
        }
    }
    return files;
}

QByteArray UbuntuProjectApp::processReservedWordsInFileName(QByteArray data, QString projectName) {
    data = data.replace(Constants::UBUNTU_FILENAME_DISPLAYNAME_UPPER,projectName.toUpper().toLatin1());
    data = data.replace(Constants::UBUNTU_FILENAME_DISPLAYNAME_LOWER,projectName.toLower().toLatin1());
    data = data.replace(Constants::UBUNTU_FILENAME_DISPLAYNAME_CAPITAL,Utils::matchCaseReplacement(QLatin1String("Capitalize"),projectName).toLatin1());
    data = data.replace(Constants::UBUNTU_FILENAME_DISPLAYNAME,projectName.toLatin1());
    return data;
}

QByteArray UbuntuProjectApp::processReservedWords(QByteArray data, QString projectPath, QString projectName) {
    QString folderName = QFileInfo(projectPath).baseName();
    data = data.replace(Constants::UBUNTU_ACTION_FOLDERNAME,folderName.toLatin1());    
    data = data.replace(Constants::UBUNTU_ACTION_DISPLAYNAME_LOWER,projectName.toLower().toLatin1());
    data = data.replace(Constants::UBUNTU_ACTION_DISPLAYNAME_UPPER,projectName.toUpper().toLatin1());
    data = data.replace(Constants::UBUNTU_ACTION_DISPLAYNAME_CAPITAL,Utils::matchCaseReplacement(QLatin1String("Capitalize"),projectName).toLatin1());
    data = data.replace(Constants::UBUNTU_ACTION_DISPLAYNAME,projectName.toLatin1());
    data = data.replace(Constants::UBUNTU_ACTION_BZR_USERNAME,m_bzrInfo.launchpadId().toLatin1());
    data = data.replace(Constants::UBUNTU_ACTION_SHAREDIRECTORY,Constants::UBUNTU_SHAREPATH.toLatin1());
    return data;
}

Core::BaseFileWizardParameters UbuntuProjectApp::parameters(QJsonObject params) {
    QString displayName, id, description, category, displayCategory;
    category = QLatin1String(Ubuntu::Constants::UBUNTU_PROJECT_WIZARD_CATEGORY);
    displayCategory = QLatin1String(Ubuntu::Constants::UBUNTU_PROJECT_WIZARD_CATEGORY_DISPLAY);

    QJsonValue tmp = params.value(QLatin1String(Constants::UBUNTU_PROJECTJSON_DISPLAYNAME));
    if (tmp.isUndefined() == false) {
        displayName = tmp.toString();
    }

    tmp = params.value(QLatin1String(Constants::UBUNTU_PROJECTJSON_ID));
    if (tmp.isUndefined() == false) {
        id = tmp.toString();
    }

    tmp = params.value(QLatin1String(Constants::UBUNTU_PROJECTJSON_DESCRIPTION));
    if (tmp.isUndefined() == false) {
        description = tmp.toString();
    }

    tmp = params.value(QLatin1String(Constants::UBUNTU_PROJECTJSON_CATEGORY));
    if (tmp.isUndefined() == false) {
        category = tmp.toString();
    }

    tmp = params.value(QLatin1String(Constants::UBUNTU_PROJECTJSON_CATEGORY_DISPLAY));
    if (tmp.isUndefined() == false) {
        displayCategory = tmp.toString();
    }

    Core::BaseFileWizardParameters parameters;
    parameters.setIcon(QIcon(QLatin1String(QtSupport::Constants::QML_WIZARD_ICON)));
    parameters.setDisplayName(displayName);
    parameters.setId(id);
    parameters.setKind(Core::IWizard::ProjectWizard);
    parameters.setFlags(Core::IWizard::PlatformIndependent);
    parameters.setDescription(description);
    parameters.setCategory(category);
    parameters.setDisplayCategory(displayCategory);
    return parameters;
}

Core::Feature UbuntuProjectApp::requiredFeature() {
    QJsonValue tmp_feature = m_obj.value(QLatin1String(Constants::UBUNTU_PROJECTJSON_REQUIRED_FEATURE));
    QString feature;
    if (tmp_feature.isUndefined() == false) {
        feature = tmp_feature.toString();
    }
    if (!feature.isEmpty()) {
        return Core::Feature(feature.toLatin1().constData());
    }
    return Core::Feature(0);
}
