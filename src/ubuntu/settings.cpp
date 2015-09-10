#include "settings.h"
#include "ubuntuconstants.h"

#include <coreplugin/icore.h>
#include <utils/fileutils.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QSettings>
#include <QString>
#include <QVariant>
#include <QDebug>
#include <QProcess>

namespace {
static const char UBUNTUSDK_DATA_KEY[] = "UbuntuSdk.";
static const char UBUNTUSDK_FILE_VERSION_KEY[] = "Version";
static const char UBUNTUSDK_FILENAME[] = "/qtcreator/ubuntu-sdk/config.xml";

static const bool DEFAULT_DEVICES_AUTOTOGGLE = true;

static const char KEY_USERNAME[] = "DeviceConnectivity.Username";
static const char KEY_IP[] = "DeviceConnectivity.IP";
static const char KEY_SSH[] = "DeviceConnectivity.SSH";
static const char KEY_AUTOTOGGLE[] = "Devices.Auto_Toggle";
static const char KEY_AUTO_CHECK_CHROOT_UPDATES[] = "Click.Auto_Check_Chroot_Updates";
static const char KEY_CHROOT_USE_LOCAL_MIRROR[] = "Click.Chroot_Use_Local_Mirror";
static const char KEY_TREAT_REVIEW_ERRORS_AS_WARNINGS[] = "ProjectDefaults.Treat_Review_Warnings_As_Errors";
static const char KEY_ENABLE_DEBUG_HELPER_DEFAULT[] = "ProjectDefaults.Enable_Debug_Helper_By_Default";
static const char KEY_UNINSTALL_APPS_FROM_DEVICE_DEFAULT[] = "ProjectDefaults.Uninstall_Apps_From_Device_By_Default";
static const char KEY_OVERRIDE_APPS_BY_DEFAULT[] = "ProjectDefaults.Override_Apps_By_Default";
}

using namespace Utils;

static FileName settingsFileName(const QString &path, QSettings::Scope scope = QSettings::UserScope)
{
    QFileInfo settingsLocation(Core::ICore::settings(scope)->fileName());
    return FileName::fromString(settingsLocation.absolutePath() + path);
}


namespace Ubuntu {
namespace Internal {

Settings *Settings::m_instance = nullptr;

Settings::Settings()
    : QObject(nullptr)
{
    Q_ASSERT_X(!m_instance, Q_FUNC_INFO, "There can be only one Settings instance");
    m_instance = this;

    //set default values
    setChrootSettings(ChrootSettings());
    setDeviceConnectivity(DeviceConnectivity());
    setProjectDefaults(ProjectDefaults());
    setDeviceAutoToggle(DEFAULT_DEVICES_AUTOTOGGLE);

    //load migrate old settings
    migrateSettings();

    //create ubuntu-sdk directory if it does not exist
    QString confdir = settingsPath().toString();
    QDir d = QDir::root();
    if(!d.exists(confdir)) {
        if(!d.mkpath(confdir))
            qWarning()<<"Unable to create Ubuntu-SDK configuration directory "<<confdir;
    }

    //tell the scripts where to find the configs
    qputenv("USDK_CONF_DIR",qPrintable(settingsPath().toString()));
}

Settings::~Settings()
{
    if (m_writer)
        delete m_writer;
}

void Settings::migrateSettings()
{
    QString oldConfDirPath = QStringLiteral("%0/.config/ubuntu-sdk").arg(QDir::homePath());
    QDir oldConfDir(oldConfDirPath);
    if (oldConfDir.exists() && !settingsPath().exists()) {
        QSettings settings(QStringLiteral("Canonical"),
                           QStringLiteral("UbuntuSDK"));

        ChrootSettings defChrootSettings;
        settings.beginGroup(QStringLiteral("Click"));

        m_settings[QLatin1String(KEY_AUTO_CHECK_CHROOT_UPDATES)]
                = settings.value(QStringLiteral("Auto_Check_Chroot_Updates"),
                                 defChrootSettings.autoCheckForUpdates).toBool();

        m_settings[QLatin1String(KEY_CHROOT_USE_LOCAL_MIRROR)]
                = settings.value(QStringLiteral("Chroot_Use_Local_Mirror"),
                                 defChrootSettings.autoCheckForUpdates).toBool();

        settings.endGroup();

        ProjectDefaults defProjectDefaults;
        settings.beginGroup(QStringLiteral("ProjectDefaults"));
        m_settings[QLatin1String(KEY_ENABLE_DEBUG_HELPER_DEFAULT)]
                = settings.value(QStringLiteral("Enable_Debug_Helper_By_Default")
                                 ,defProjectDefaults.enableDebugHelper).toBool();

        m_settings[QLatin1String(KEY_OVERRIDE_APPS_BY_DEFAULT)]
                = settings.value(QStringLiteral("Override_Apps_By_Default")
                                 ,defProjectDefaults.overrideAppsByDefault).toBool();

        m_settings[QLatin1String(KEY_UNINSTALL_APPS_FROM_DEVICE_DEFAULT)]
                = settings.value(QStringLiteral("Uninstall_Apps_From_Device_By_Default")
                                 ,defProjectDefaults.uninstallAppsByDefault).toBool();

        m_settings[QLatin1String(KEY_TREAT_REVIEW_ERRORS_AS_WARNINGS)]
                = settings.value(QStringLiteral("Treat_Review_Warnings_As_Errors")
                                 ,defProjectDefaults.reviewErrorsAsWarnings).toBool();
        settings.endGroup();

        FileName parentDir = settingsPath().parentDir();
        if (!parentDir.exists()) {
            QDir d;
            if (!d.mkpath(parentDir.toString())) {
                qWarning()<<"Could not create settings directory";
                return;
            }
        }

        //moving the old settings directory is enough, the ubuntu kit manager
        //will fix the scriptpaths in the existing Kits later
        if (!QDir().rename(oldConfDirPath,settingsPath().toString())) {
            QStringList args = {
                oldConfDirPath,
                settingsPath().toString()
            };

            if (QProcess::execute(QStringLiteral("mv"), args) != 0)
                qWarning()<<"Could not rename the old settings directory to "<<settingsPath().toString();
        }
    }
}

FileName Settings::settingsPath()
{
    return FileName::fromString(
                settingsFileName(QLatin1String(UBUNTUSDK_FILENAME))
                .toFileInfo()
                .absolutePath());
}

void Settings::restoreSettings()
{
    QTC_ASSERT(!m_writer, return);
    m_writer = new PersistentSettingsWriter(
                settingsFileName(QLatin1String(UBUNTUSDK_FILENAME)),
                QLatin1String("UbuntuSDKSettings"));

    PersistentSettingsReader read;
    if (read.load(settingsFileName(QLatin1String(UBUNTUSDK_FILENAME), QSettings::SystemScope)))
        m_settings = read.restoreValues();

    //load from user scope override system settings
    if (read.load(settingsFileName(QLatin1String(UBUNTUSDK_FILENAME)))) {
        QVariantMap userSettings = read.restoreValues();
        foreach (const QString &key, userSettings.keys())
            m_settings[key] = userSettings[key];
    }

    connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
            this, &Settings::flushSettings);
}

void Settings::flushSettings()
{
    m_instance->m_settings[QLatin1String(UBUNTUSDK_FILE_VERSION_KEY)] = 1;
    m_instance->m_writer->save(m_instance->m_settings, Core::ICore::mainWindow());
}

Settings::DeviceConnectivity Settings::deviceConnectivity()
{
    DeviceConnectivity val;
    val.ip = m_instance->m_settings.value(QLatin1String(KEY_IP),val.ip).toString();
    val.user = m_instance->m_settings.value(QLatin1String(KEY_USERNAME),val.user).toString();
    val.sshPort = m_instance->m_settings.value(QLatin1String(KEY_SSH),val.sshPort).toInt();
    return val;
}

void Settings::setDeviceConnectivity(const Settings::DeviceConnectivity &settings)
{
    m_instance->m_settings[QLatin1String(KEY_IP)] = settings.ip;
    m_instance->m_settings[QLatin1String(KEY_USERNAME)] = settings.user;
    m_instance->m_settings[QLatin1String(KEY_SSH)] = settings.sshPort;
}

Settings::ProjectDefaults Settings::projectDefaults()
{
    ProjectDefaults defaults;
    defaults.enableDebugHelper = m_instance->m_settings.value(QLatin1String(KEY_ENABLE_DEBUG_HELPER_DEFAULT),
                                                  defaults.enableDebugHelper).toBool();
    defaults.overrideAppsByDefault = m_instance->m_settings.value(QLatin1String(KEY_OVERRIDE_APPS_BY_DEFAULT),
                                                  defaults.overrideAppsByDefault).toBool();
    defaults.reviewErrorsAsWarnings = m_instance->m_settings.value(QLatin1String(KEY_TREAT_REVIEW_ERRORS_AS_WARNINGS),
                                                  defaults.reviewErrorsAsWarnings).toBool();
    defaults.uninstallAppsByDefault = m_instance->m_settings.value(QLatin1String(KEY_UNINSTALL_APPS_FROM_DEVICE_DEFAULT),
                                                  defaults.uninstallAppsByDefault).toBool();
    return defaults;
}

void Settings::setProjectDefaults(const Settings::ProjectDefaults &settings)
{
    m_instance->m_settings[QLatin1String(KEY_ENABLE_DEBUG_HELPER_DEFAULT)] = settings.enableDebugHelper;
    m_instance->m_settings[QLatin1String(KEY_OVERRIDE_APPS_BY_DEFAULT)] = settings.overrideAppsByDefault;
    m_instance->m_settings[QLatin1String(KEY_TREAT_REVIEW_ERRORS_AS_WARNINGS)] = settings.reviewErrorsAsWarnings;
    m_instance->m_settings[QLatin1String(KEY_UNINSTALL_APPS_FROM_DEVICE_DEFAULT)] = settings.uninstallAppsByDefault;
}

Settings::ChrootSettings Settings::chrootSettings()
{
    ChrootSettings val;
    val.autoCheckForUpdates = m_instance->m_settings.value(QLatin1String(KEY_AUTO_CHECK_CHROOT_UPDATES),val.autoCheckForUpdates).toBool();
    val.useLocalMirror = m_instance->m_settings.value(QLatin1String(KEY_CHROOT_USE_LOCAL_MIRROR),val.useLocalMirror).toBool();
    return val;
}

void Settings::setChrootSettings(const Settings::ChrootSettings &settings)
{
    m_instance->m_settings[QLatin1String(KEY_AUTO_CHECK_CHROOT_UPDATES)] = settings.autoCheckForUpdates;
    m_instance->m_settings[QLatin1String(KEY_CHROOT_USE_LOCAL_MIRROR)]    = settings.useLocalMirror;
}

bool Settings::deviceAutoToggle()
{
    return m_instance->m_settings.value(QLatin1String(KEY_AUTOTOGGLE),
                            DEFAULT_DEVICES_AUTOTOGGLE).toBool();
}

void Settings::setDeviceAutoToggle(const bool set)
{
    m_instance->m_settings[QLatin1String(KEY_AUTOTOGGLE)] = set;
}

} // namespace Internal
} // namespace Ubuntu
