#include "ubuntucmakecache.h"

#include <projectexplorer/session.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildconfiguration.h>
#include <cmakeprojectmanager/cmakeprojectconstants.h>

#include <QRegularExpression>
#include <QDir>
#include <QDebug>

namespace Ubuntu{
namespace Internal {

enum {
    debug = 0
};

/*!
 * \class UbuntuCMakeCache::UbuntuCMakeCache
 * Automatic updating value cache, this reads all relevant CMakeCache files
 * and stores some for quick querying
 */

UbuntuCMakeCache * UbuntuCMakeCache::m_instance = nullptr;

UbuntuCMakeCache::UbuntuCMakeCache(QObject *parent) :
    QObject(parent)
{
    Q_ASSERT_X(m_instance == nullptr,Q_FUNC_INFO,"There can be only one UbuntuCMakeCache instance");
    m_instance = this;

    connect(ProjectExplorer::SessionManager::instance(),SIGNAL(aboutToRemoveProject(ProjectExplorer::Project *)),
            this,SLOT(onAboutToRemoveProject(ProjectExplorer::Project*)));
}

QVariant UbuntuCMakeCache::getValue(const QString &key, ProjectExplorer::BuildConfiguration *bc, const QVariant defaultValue)
{
    UbuntuCMakeCache *inst = instance();
    if(!inst || !bc)
        return defaultValue;

    Utils::FileName cacheFile = bc->buildDirectory().appendPath(QStringLiteral("CMakeCache.txt"));
    QString cacheKey = inst->normalize(cacheFile.toString());

    bool needsRefresh = false;
    if(!inst->m_map.contains(cacheKey)) {
        //we need to read it
        needsRefresh = true;
    } else {
        //check if the file has changed since the last read
        inst->m_map[cacheKey].cacheFile.refresh();
        if(debug) qDebug()<<"File Last Read "<<inst->m_map[cacheKey].cacheFile.lastModified()
                         <<"Cache Last Read "<<inst->m_map[cacheKey].lastRead;
        if(inst->m_map[cacheKey].cacheFile.lastModified() != inst->m_map[cacheKey].lastRead) {
            needsRefresh = true;
        }
    }

    if(needsRefresh)
        inst->refreshCache(cacheKey, cacheFile);

    if(inst->m_map[cacheKey].values.contains(key))
        return inst->m_map[cacheKey].values[key];

    return defaultValue;
}

UbuntuCMakeCache *UbuntuCMakeCache::instance()
{
    return m_instance;
}

void UbuntuCMakeCache::onAboutToRemoveProject(ProjectExplorer::Project *p)
{
    if(!p)
        return;

    //remove all values that belong to the project
    for (ProjectExplorer::Target *t : p->targets()) {
        for(ProjectExplorer::BuildConfiguration *bc : t->buildConfigurations()) {
            Utils::FileName cacheFile = bc->buildDirectory().appendPath(QStringLiteral("CMakeCache.txt"));
            m_map.remove(normalize(cacheFile.toString()));
        }
    }
}

QString UbuntuCMakeCache::normalize(const QString &path) const
{
    return QDir::cleanPath(path);
}

void UbuntuCMakeCache::refreshCache(const QString &key, const Utils::FileName &fName)
{
    if(debug) qDebug()<<"Rebuilding cache for: "<<key;
    UbuntuCMakeCacheEntry *entry = nullptr;
    if(m_map.contains(key)) {
        entry = &m_map[key];
        entry->lastRead = fName.toFileInfo().lastModified();
    } else {
        QFileInfo fInfo = fName.toFileInfo();
        entry = &m_map.insert(key,UbuntuCMakeCacheEntry{fInfo,fInfo.lastModified(),CMakeCacheValueMap()}).value();
    }

    QFile cache(fName.toString());
    if(cache.exists() && cache.open(QIODevice::ReadOnly)) {
        static const QRegularExpression regExpPType(QLatin1String("^UBUNTU_PROJECT_TYPE:(.*)=\\s*(\\S*)\\s*$"));
        static const QRegularExpression regExpManifestPath (QLatin1String("^UBUNTU_MANIFEST_PATH:(.*)=\\s*(\\S*)\\s*$"));
        QTextStream in(&cache);
        while (!in.atEnd()) {
            QString contents = in.readLine();
            QRegularExpressionMatch m = regExpPType.match(contents);
            if(m.hasMatch()) {
                entry->values.insert(QStringLiteral("UBUNTU_PROJECT_TYPE"),m.captured(2));
                continue;
            }
            m = regExpManifestPath.match(contents);
            if(m.hasMatch()) {
                entry->values.insert(QStringLiteral("UBUNTU_MANIFEST_PATH"),m.captured(2));
                continue;
            }
        }


    }
}

}}

