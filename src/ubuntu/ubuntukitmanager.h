#ifndef UBUNTU_INTERNAL_UBUNTUKITMANAGER_H
#define UBUNTU_INTERNAL_UBUNTUKITMANAGER_H

#include "clicktoolchain.h"
#include <projectexplorer/kit.h>

namespace Debugger{
class DebuggerItem;
}

namespace Ubuntu {
namespace Internal {

class UbuntuKitManager : public QObject
{
    Q_OBJECT
public:
    UbuntuKitManager();

    static void autoDetectKits ();
    static ProjectExplorer::Kit *createKit (ClickToolChain* tc);
    static QVariant createOrFindDebugger(const Utils::FileName &path);
    static void fixKit (ProjectExplorer::Kit* k);
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUKITMANAGER_H
