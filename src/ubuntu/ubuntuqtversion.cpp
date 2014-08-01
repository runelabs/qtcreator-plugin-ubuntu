#include "ubuntuqtversion.h"
#include "ubuntuconstants.h"

#include <qtsupport/qtsupportconstants.h>

#include <QCoreApplication>
#include <QFileInfo>

namespace Ubuntu {
namespace Internal {

UbuntuQtVersion::UbuntuQtVersion()
    : BaseQtVersion()
{ }

UbuntuQtVersion::UbuntuQtVersion(const Utils::FileName &path, bool isAutodetected, const QString &autodetectionSource)
    : BaseQtVersion(path, isAutodetected, autodetectionSource)
{
    setDisplayName(defaultDisplayName(qtVersionString(), path, false));
}

UbuntuQtVersion::~UbuntuQtVersion()
{ }

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
    if(!qmakePath.toFileInfo().absolutePath().endsWith(QStringLiteral(".config/ubuntu-sdk")))
        return 0;

    return new UbuntuQtVersion(qmakePath,isAutoDetected,autoDetectionSource);
}

} // namespace Internal
} // namespace Ubuntu
