#ifndef UBUNTUCMAKECACHE_H
#define UBUNTUCMAKECACHE_H

#include <QObject>
#include <QHash>
#include <QMap>
#include <QVariant>
#include <QDateTime>
#include <QFileInfo>

#include <utils/fileutils.h>

namespace ProjectExplorer {
    class Project;
    class Target;
    class BuildConfiguration;
}

namespace Ubuntu{
namespace Internal {

typedef QMap<QString,QVariant> CMakeCacheValueMap;

struct UbuntuCMakeCacheEntry {
    QFileInfo cacheFile;
    QDateTime lastRead;
    CMakeCacheValueMap values;
};

typedef QMap<QString,UbuntuCMakeCacheEntry> CMakeCache;

class UbuntuCMakeCache : public QObject
{
    Q_OBJECT
public:
    explicit UbuntuCMakeCache(QObject *parent = 0);

    static QVariant getValue (const QString &key, ProjectExplorer::BuildConfiguration *bc, const QVariant defaultValue = QVariant());
    static UbuntuCMakeCache *instance();

private slots:
    void onAboutToRemoveProject (ProjectExplorer::Project *p);

private:
    QString normalize (const QString &path) const;
    void refreshCache (const QString &key, const Utils::FileName &fName);

private:
    CMakeCache m_map;
    static UbuntuCMakeCache *m_instance;

};

}}



#endif // UBUNTUCMAKECACHE_H
