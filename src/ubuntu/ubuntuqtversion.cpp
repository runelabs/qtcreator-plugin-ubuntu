#include "ubuntuqtversion.h"
#include "ubuntuconstants.h"
#include "settings.h"

#include <qtsupport/qtsupportconstants.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QDebug>

namespace Ubuntu {
namespace Internal {

const char SCRIPT_VERSION_KEY[] = "UbuntuQtVersion.ScriptVersion";

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

UbuntuQtVersion::UbuntuQtVersion(const Utils::FileName &path, bool isAutodetected, const QString &autodetectionSource)
    : BaseQtVersion(path, isAutodetected, autodetectionSource),
      m_scriptVersion(MIN_SCRIPT_VERSION)
{
    setUnexpandedDisplayName(defaultUnexpandedDisplayName(path, false));
}

UbuntuQtVersion::~UbuntuQtVersion()
{}

void UbuntuQtVersion::fromMap(const QVariantMap &map)
{
    BaseQtVersion::fromMap(map);
    m_scriptVersion = map.value(QLatin1String(SCRIPT_VERSION_KEY),0).toInt();
}

QVariantMap UbuntuQtVersion::toMap() const
{
    QVariantMap map = BaseQtVersion::toMap();
    map.insert(QLatin1String(SCRIPT_VERSION_KEY),m_scriptVersion);
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

QString UbuntuQtVersion::platformName() const
{
    return QLatin1String(Constants::UBUNTU_PLATFORM_NAME);
}

QString UbuntuQtVersion::platformDisplayName() const
{
    return QLatin1String(Constants::UBUNTU_PLATFORM_NAME_TR);
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
    if(!qmakePath.toFileInfo().absolutePath().contains(Settings::settingsPath().toString()))
        return 0;

    return new UbuntuQtVersion(qmakePath,isAutoDetected,autoDetectionSource);
}

} // namespace Internal
} // namespace Ubuntu
