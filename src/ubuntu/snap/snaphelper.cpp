#include "snaphelper.h"

#include <projectexplorer/project.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

namespace Ubuntu {
namespace Internal {

SnapHelper::SnapHelper()
{

}

bool SnapHelper::isSnappyProject(ProjectExplorer::Project *project)
{
    if (!project)
        return false;

    QString mimeType = project->projectManager()->mimeType();
    if(mimeType != QLatin1String(QmakeProjectManager::Constants::PROFILE_MIMETYPE))
        return false;

    QmakeProjectManager::QmakeProject *qPro = static_cast<QmakeProjectManager::QmakeProject *>(project);
    QmakeProjectManager::QmakeProFileNode *node = qPro->rootProjectNode();
    return node->variableValue(QmakeProjectManager::ConfigVar).contains(QStringLiteral("snapcraft"), Qt::CaseInsensitive);
}

} // namespace Internal
} // namespace Ubuntu
