#ifndef UBUNTU_INTERNAL_UBUNTUPROJECTGUESSER_H
#define UBUNTU_INTERNAL_UBUNTUPROJECTGUESSER_H

#include <utils/fileutils.h>
#include <QRegularExpression>


namespace ProjectExplorer {
    class Project;
}

namespace Ubuntu {
namespace Internal {

class UbuntuProjectGuesser
{
public:
    UbuntuProjectGuesser();

    static bool isScopesProject    ( ProjectExplorer::Project* project, QString *iniFileName = 0 );
    static bool isClickAppProject  ( ProjectExplorer::Project *project);


    static Utils::FileName findScopesIniRecursive (const Utils::FileName &searchdir);
    static Utils::FileName findFileRecursive (const Utils::FileName &searchdir, const QString &regexp);
    static Utils::FileName findFileRecursive (const Utils::FileName &searchdir, const QRegularExpression &regexp);

};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUPROJECTGUESSER_H
