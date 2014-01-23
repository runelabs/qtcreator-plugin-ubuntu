#ifndef UBUNTU_INTERNAL_UBUNTUCLICKTOOL_H
#define UBUNTU_INTERNAL_UBUNTUCLICKTOOL_H

#include <QList>
#include <QString>

#include <projectexplorer/processparameters.h>
#include <utils/qtcprocess.h>

namespace Ubuntu {
namespace Internal {

class UbuntuClickTool
{
public:

    enum MaintainMode {
        Upgrade,//runs click chroot upgrade
        Delete  //runs click chroot delete
    };

    struct Target {
        QString framework;
        QString architecture;
    };

    UbuntuClickTool();

    void parametersForCreateChroot   (const QString &arch, const QString &series,ProjectExplorer::ProcessParameters* params);
    void parametersForMaintainChroot (const MaintainMode &mode,const Target& target,ProjectExplorer::ProcessParameters* params);
    void parametersForCmake        (const Target& target, const QString &buildDir
                                    , const QString &relPathToSource,ProjectExplorer::ProcessParameters* params);
    void parametersForMake         (const Target& target, const QString &buildDir,ProjectExplorer::ProcessParameters* params);

    static QList<Target> listAvailableTargets ();


private:
    Utils::QtcProcess* m_clickProcess;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUCLICKTOOL_H
