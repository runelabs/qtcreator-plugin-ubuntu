#ifndef UBUNTU_INTERNAL_UBUNTUFIXMANIFESTSTEP_H
#define UBUNTU_INTERNAL_UBUNTUFIXMANIFESTSTEP_H

#include <projectexplorer/buildstep.h>

namespace Ubuntu {
namespace Internal {

class UbuntuFixManifestStep : public ProjectExplorer::BuildStep
{
public:
    UbuntuFixManifestStep(ProjectExplorer::BuildStepList *bsl);

    QStringList architectures() const;
    void setArchitectures(const QStringList &architectures);

    QString packageDir() const;
    void setPackageDir(const QString &packageDir);

    // BuildStep interface
    virtual bool init(QList<const BuildStep *> &earlierSteps) override;
    virtual void run(QFutureInterface<bool> &fi) override;
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;

private:
    QString m_packageDir;
    QStringList m_architectures;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUFIXMANIFESTSTEP_H
