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

#include "ubuntuproject.h"
#include <coreplugin/modemanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/projectmacroexpander.h>
#include <qmljs/qmljssimplereader.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>


using namespace Ubuntu;
using namespace Ubuntu::Internal;

UbuntuProject::UbuntuProject(UbuntuProjectManager *manager, const QString &fileName)
    : m_manager(manager),
      m_fileName(fileName) {

    setId(Constants::UBUNTUPROJECT_ID);
    setRequiredKitMatcher(UbuntuKitMatcher());
    setPreferredKitMatcher(QtSupport::QtKitInformation::qtVersionMatcher(Core::FeatureSet(QtSupport::Constants::FEATURE_DESKTOP)));

    setProjectContext(Core::Context(Constants::UBUNTUPROJECT_PROJECTCONTEXT));

    QFileInfo fileInfo(m_fileName);
    m_projectName = fileInfo.completeBaseName();

    m_file = QSharedPointer<UbuntuProjectFile>(new UbuntuProjectFile(this, fileName));

    Core::DocumentManager::addDocument(m_file.data(), true);

    m_rootNode = QSharedPointer<UbuntuProjectNode>(new UbuntuProjectNode(this, m_file.data()));
    m_manager->registerProject(this);

    extractProjectFileData(fileName);
}

void UbuntuProject::extractProjectFileData(const QString& filename) {
    QmlJS::SimpleReader reader;

    const QmlJS::SimpleReaderNode::Ptr root =
            reader.readFile(filename);

    if(!reader.errors().isEmpty()) {
        foreach(const QString &error, reader.errors()) {
            qWarning()<<qPrintable(tr("Error in projectfile: %0").arg(error));
        }
        return;
    }

    if(root.isNull())
        return;

    if (root->name().compare(QString::fromLatin1("Project")) == 0) {
        QVariant mainFileVariant = root->property(QLatin1String("mainFile"));
        if (mainFileVariant.isValid())
            m_mainFile = mainFileVariant.toString();
    } else  {
        qWarning()<< tr("There is no Project root element in the projectfile");
    }
}

QString UbuntuProject::displayName() const {
    return m_projectName;
}

Core::IDocument *UbuntuProject::document() const {
    return m_file.data();
}

ProjectExplorer::IProjectManager *UbuntuProject::projectManager() const {
    return m_manager;
}

ProjectExplorer::ProjectNode *UbuntuProject::rootProjectNode() const {
    return m_rootNode.data();
}

static void enumChild(const QDir &dir, QStringList &res) {
    foreach (const QFileInfo &info, dir.entryInfoList(QDir::NoDotAndDotDot|QDir::Dirs|QDir::Files|QDir::Hidden)) {
        if (info.fileName().indexOf(QLatin1String(Constants::UBUNTUPROJECT_SUFFIX)) != -1
            || info.fileName().indexOf(QLatin1String(Constants::UBUNTUHTMLPROJECT_SUFFIX)) != -1)
            continue;
        if (info.isFile()) {
            if(info.isHidden() && info.fileName() != QLatin1String(".excludes"))
                continue;
            res.append(info.absoluteFilePath());
        } else if (info.isDir()) {
            enumChild(QDir(info.absoluteFilePath()), res);
        }
    }
}

QStringList UbuntuProject::files(FilesMode) const {
    QStringList files;
    enumChild(projectDir(), files);
    return files;
}

bool UbuntuProject::supportsKit(ProjectExplorer::Kit *k, QString *errorMessage) const
{
    UbuntuKitMatcher matcher;
    if (!matcher.matches(k)) {
        if(errorMessage)
            *errorMessage = tr("Only Desktop and Ubuntu Kits are supported");
        return false;
    }

    return true;
}

bool UbuntuProject::needsConfiguration() const
{
    return targets().size() == 0;
}

bool UbuntuProject::requiresTargetPanel() const
{
    return true;
}

QString UbuntuProject::shadowBuildDirectory(const QString &proFilePath, const ProjectExplorer::Kit *k, const QString &suffix)
{
    if (proFilePath.isEmpty())
        return QString();

    QFileInfo info(proFilePath);

    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(k);
    if (version)
        return info.absolutePath();

    const QString projectName = QFileInfo(proFilePath).completeBaseName();
    ProjectExplorer::ProjectMacroExpander expander(projectName, k, suffix);
    QDir projectDir = QDir(projectDirectory(Utils::FileName::fromString(proFilePath)).toString());
    QString buildPath = expander.expand(Core::DocumentManager::buildDirectory());
    return QDir::cleanPath(projectDir.absoluteFilePath(buildPath));
}


UbuntuKitMatcher::UbuntuKitMatcher()
    : KitMatcher(&UbuntuKitMatcher::matches)
{
}

bool UbuntuKitMatcher::matches(const ProjectExplorer::Kit *k)
{
    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(k);
    if (tc->type() == QLatin1String(Ubuntu::Constants::UBUNTU_CLICK_TOOLCHAIN_ID))
        return true;

    if (ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(k) == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
        return true;

    return false;
}
