#include "ubuntuqtversion.h"
#include "ubuntuconstants.h"
#include "settings.h"

#include <ubuntu/device/container/containerdevice.h>
#include <ubuntu/ubuntuclicktool.h>
#include <qtsupport/qtsupportconstants.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QDebug>
#include <QDir>

namespace Ubuntu {
namespace Internal {

const char SCRIPT_VERSION_KEY[] = "UbuntuQtVersion.ScriptVersion";
const char CONTAINER_NAME[] = "UbuntuQtVersion.ContainerName";

/*!
 * \brief MIN_SCRIPT_VERSION
 * Increment this version if all qmake scripts in <confdir>/ubuntu-sdk
 * need to be recreated
 */
const int  MIN_SCRIPT_VERSION   = 2;

UbuntuQtVersion::UbuntuQtVersion()
    : BaseQtVersion(),
      m_scriptVersion(MIN_SCRIPT_VERSION)
{ }

UbuntuQtVersion::UbuntuQtVersion(const QString &containerName, const Utils::FileName &path, bool isAutodetected, const QString &autodetectionSource)
    : BaseQtVersion(path, isAutodetected, autodetectionSource),
      m_scriptVersion(MIN_SCRIPT_VERSION),
      m_containerName(containerName)
{
    setUnexpandedDisplayName(defaultUnexpandedDisplayName(path, false));
}

UbuntuQtVersion::~UbuntuQtVersion()
{}

void UbuntuQtVersion::fromMap(const QVariantMap &map)
{
    BaseQtVersion::fromMap(map);
    m_scriptVersion = map.value(QLatin1String(SCRIPT_VERSION_KEY),0).toInt();
    m_containerName = map.value(QLatin1String(CONTAINER_NAME),QString()).toString();

    if (m_containerName.isEmpty()) {
        //ok, this is a old QtVersion, we need to restore the container name from the
        //qmake path
        Utils::FileName command = this->qmakeCommand();
        m_containerName = command.toFileInfo().dir().dirName();
    }
}

QVariantMap UbuntuQtVersion::toMap() const
{
    QVariantMap map = BaseQtVersion::toMap();
    map.insert(QLatin1String(SCRIPT_VERSION_KEY),m_scriptVersion);
    map.insert(QLatin1String(CONTAINER_NAME), m_containerName);
    return map;
}

UbuntuQtVersion *UbuntuQtVersion::clone() const
{
    return new UbuntuQtVersion(*this);
}

QString UbuntuQtVersion::type() const
{
    return QLatin1String(Constants::UBUNTU_QTVERSION_TYPE);
}

QList<ProjectExplorer::Abi> UbuntuQtVersion::detectQtAbis() const
{
    return qtAbisFromLibrary(qtCorePaths(versionInfo(), qtVersionString()));
}

QString UbuntuQtVersion::description() const
{
    return QCoreApplication::translate("QtVersion", "Ubuntu Phone", "Qt Version is used for Ubuntu Phone development");
}

QSet<Core::Id> UbuntuQtVersion::targetDeviceTypes() const
{
    QSet<Core::Id> set{
        Constants::UBUNTU_DEVICE_TYPE_ID
    };

    auto hostAbi = ProjectExplorer::Abi::hostAbi();
    for (const ProjectExplorer::Abi &abi : qtAbis()) {
        if (abi.architecture() == hostAbi.architecture() &&
                abi.os() == hostAbi.os()) {
            set << ContainerDevice::createIdForContainer(m_containerName);
        }
    }

    return set;
}

int UbuntuQtVersion::scriptVersion() const
{
    return m_scriptVersion;
}

void UbuntuQtVersion::setScriptVersion(int scriptVersion)
{
    m_scriptVersion = scriptVersion;
}

int UbuntuQtVersion::minimalScriptVersion()
{
    return MIN_SCRIPT_VERSION;
}

QString UbuntuQtVersion::remoteQMakeCommand() const
{
    return QString::fromLatin1("/usr/bin/%2").arg(qmakeCommand().fileName());
}

bool UbuntuQtVersion::hasQmlDump() const
{
    return false;
}

bool UbuntuQtVersion::hasQmlDumpWithRelocatableFlag() const
{
    return false;
}

bool UbuntuQtVersion::needsQmlDump() const
{
    return false;
}


bool UbuntuQtVersionFactory::canRestore(const QString &type)
{
    return type == QLatin1String(Constants::UBUNTU_QTVERSION_TYPE);
}

QtSupport::BaseQtVersion *UbuntuQtVersionFactory::restore(const QString &type, const QVariantMap &data)
{
    if(!canRestore(type))
        return 0;

    UbuntuQtVersion *v = new UbuntuQtVersion();
    v->fromMap(data);
    return v;
}

int UbuntuQtVersionFactory::priority() const
{
    return 100;
}

QtSupport::BaseQtVersion *UbuntuQtVersionFactory::create(const Utils::FileName &qmakePath, ProFileEvaluator *evaluator, bool isAutoDetected, const QString &autoDetectionSource)
{
    Q_UNUSED(evaluator);
    //we only care about our qmakes
    QFileInfo qmakeInfo = qmakePath.toFileInfo();
    if(!qmakeInfo.absolutePath().contains(Settings::settingsPath().toString()))
        return 0;

    if(!qmakeInfo.isSymLink() || qmakeInfo.symLinkTarget() != Constants::UBUNTU_CLICK_TARGET_WRAPPER)
        return 0;

    QString containerName = qmakePath.toFileInfo().dir().dirName();
    if (!UbuntuClickTool::targetExists(containerName))
        return 0;

    return new UbuntuQtVersion(containerName, qmakePath,isAutoDetected,autoDetectionSource);
}

} // namespace Internal
} // namespace Ubuntu
