#ifndef UBUNTU_INTERNAL_SNAPHELPER_H
#define UBUNTU_INTERNAL_SNAPHELPER_H

namespace ProjectExplorer {
class Project;
}

namespace Ubuntu {
namespace Internal {

class SnapHelper
{
public:
    SnapHelper();

    static bool isSnappyProject (ProjectExplorer::Project* project);
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_SNAPHELPER_H
