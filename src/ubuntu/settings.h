#ifndef UBUNTU_INTERNAL_SETTINGS_H
#define UBUNTU_INTERNAL_SETTINGS_H

#include <utils/fileutils.h>

#include <QObject>
#include <QVariantMap>

namespace Utils { class PersistentSettingsWriter; }

namespace Ubuntu {
namespace Internal {

class Settings : public QObject
{
    Q_OBJECT
public:

    struct DeviceConnectivity {
        QString user = QStringLiteral("phablet");
        QString ip   = QStringLiteral("127.0.0.1");
        int sshPort  = 2222;
    };

    struct ProjectDefaults {
        bool reviewErrorsAsWarnings = false;
        bool enableDebugHelper      = true;
        bool uninstallAppsByDefault = true;
        bool overrideAppsByDefault  = false;
    };

    struct ChrootSettings {
        bool useLocalMirror = false;
        bool autoCheckForUpdates = true;
    };

    explicit Settings();
    virtual ~Settings();

    void restoreSettings ();
    static void flushSettings ();

    static Utils::FileName settingsPath ();

    static DeviceConnectivity deviceConnectivity ();
    static void setDeviceConnectivity (const DeviceConnectivity &settings);

    static ProjectDefaults projectDefaults ();
    static void setProjectDefaults (const ProjectDefaults &settings);

    static ChrootSettings chrootSettings ();
    static void setChrootSettings (const ChrootSettings &settings);

    static bool deviceAutoToggle ();
    static void setDeviceAutoToggle (const bool set);

private:
    void migrateSettings ();

private:
    static Settings *m_instance;
    QVariantMap m_settings;
    Utils::PersistentSettingsWriter *m_writer = 0;

};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_SETTINGS_H
