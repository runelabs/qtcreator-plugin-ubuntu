#ifndef UBUNTU_INTERNAL_UBUNTUKITMANAGER_H
#define UBUNTU_INTERNAL_UBUNTUKITMANAGER_H

#include "clicktoolchain.h"
#include <projectexplorer/kit.h>

namespace Ubuntu {
namespace Internal {

class UbuntuKitManager : public QObject
{
    Q_OBJECT
public:
    UbuntuKitManager();

    static void autoDetectKits ();
    static ProjectExplorer::Kit *createKit (ClickToolChain* tc);
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUKITMANAGER_H
