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
#include <qmljs/qmljssimplereader.h>


using namespace Ubuntu;
using namespace Ubuntu::Internal;

UbuntuProject::UbuntuProject(UbuntuProjectManager *manager, const QString &fileName)
    : m_manager(manager),
      m_fileName(fileName) {

    setProjectContext(Core::Context(Constants::UBUNTUPROJECT_PROJECTCONTEXT));

    QFileInfo fileInfo(m_fileName);
    m_projectName = fileInfo.completeBaseName();

    m_file = QSharedPointer<UbuntuProjectFile>(new UbuntuProjectFile(this, fileName));

    Core::DocumentManager::addDocument(m_file.data(), true);

    m_rootNode = QSharedPointer<UbuntuProjectNode>(new UbuntuProjectNode(this, m_file.data()));
    m_manager->registerProject(this);

    QList<ProjectExplorer::Kit *> kits = ProjectExplorer::KitManager::instance()->kits();
    foreach (ProjectExplorer::Kit *kit, kits) {
        addTarget(createTarget(kit));
    }

    if (needsConfiguration()) {
        Core::ModeManager::activateMode(ProjectExplorer::Constants::MODE_SESSION);
    }

    extractProjectFileData(fileName);
}

void UbuntuProject::extractProjectFileData(const QString& filename) {
    QmlJS::SimpleReader reader;

    const QmlJS::SimpleReaderNode::Ptr root =
            reader.readFile(filename);

    if (!reader.errors().isEmpty() || root.isNull())
        return;

    if (root->name().compare(QString::fromLatin1("Project")) == 0) {
        QVariant mainFileVariant = root->property(QLatin1String("mainFile"));
        if (mainFileVariant.isValid())
            m_mainFile = mainFileVariant.toString();
    }
}

QString UbuntuProject::displayName() const {
    return m_projectName;
}

Core::Id UbuntuProject::id() const {
    return Core::Id(Constants::UBUNTUPROJECT_ID);
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
    foreach (const QFileInfo &info, dir.entryInfoList(QDir::NoDotAndDotDot|QDir::Dirs|QDir::Files)) {
        if (info.fileName().indexOf(QLatin1String(Constants::UBUNTUPROJECT_SUFFIX)) != -1
            || info.fileName().indexOf(QLatin1String(Constants::UBUNTUHTMLPROJECT_SUFFIX)) != -1)
            continue;
        if (info.isFile()) {
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
