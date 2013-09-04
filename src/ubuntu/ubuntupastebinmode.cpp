#include "ubuntupastebinmode.h"
#include "ubuntuconstants.h"

using namespace Ubuntu::Internal;

UbuntuPastebinMode::UbuntuPastebinMode(QObject *parent) :
    UbuntuWebMode(parent)
{
    setDisplayName(tr(Ubuntu::Constants::UBUNTU_MODE_PASTEBIN_DISPLAYNAME));
    setId(Ubuntu::Constants::UBUNTU_MODE_PASTEBIN);
    setObjectName(QLatin1String(Ubuntu::Constants::UBUNTU_MODE_PASTEBIN));

    setUrl(QUrl(QLatin1String(Ubuntu::Constants::UBUNTU_PASTEBIN)));
}
