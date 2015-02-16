#ifndef UBUNTU_INTERNAL_UBUNTUFATPACKAGINGWIZARD_H
#define UBUNTU_INTERNAL_UBUNTUFATPACKAGINGWIZARD_H

#include <utils/wizard.h>

#include <QList>

class QLabel;
class QComboBox;
class QTreeWidget;
class QTreeWidgetItem;
namespace ProjectExplorer{ class Project; class BuildConfiguration; }
namespace QmakeProjectManager { class QmakeProject; }
namespace Utils { class PathChooser; }

namespace Ubuntu {
namespace Internal {

class UbuntuChooseTargetPage;

class UbuntuFatPackagingWizard : public Utils::Wizard
{
    Q_OBJECT
public:
    explicit UbuntuFatPackagingWizard(QmakeProjectManager::QmakeProject *project, QWidget *parent = 0);
    QList<ProjectExplorer::BuildConfiguration *> selectedTargets();

signals:

public slots:

private:
    UbuntuChooseTargetPage *m_targetPage;
};

class UbuntuFatPackagingIntroPage : public QWizardPage
{
    Q_OBJECT
public:
    UbuntuFatPackagingIntroPage (QmakeProjectManager::QmakeProject *project, QWidget *parent = 0);

    // QWizardPage interface
    virtual void initializePage() override;
    virtual bool isComplete() const override;
    virtual int nextId() const override;

protected slots:
    void buildModeChanged ( const int mode );

private:
    QmakeProjectManager::QmakeProject *m_project;
    QComboBox *m_modeBox;
    Utils::PathChooser *m_clickPackagePath;
};

class UbuntuChooseTargetPage : public QWizardPage
{
    Q_OBJECT
public:
    UbuntuChooseTargetPage (QmakeProjectManager::QmakeProject *project, QWidget *parent = 0);

    // QWizardPage interface
    virtual void initializePage() override;
    virtual bool isComplete() const override;
    virtual bool validatePage() override;

    QList<ProjectExplorer::BuildConfiguration *> selectedSuspects() const;

private:
    bool isValid () const;

private:
    QmakeProjectManager::QmakeProject *m_project;
    mutable QLabel *m_errorLabel;
    QLabel *m_noTargetsLabel;
    QTreeWidget *m_targetView;
    QList<ProjectExplorer::BuildConfiguration *> m_suspects;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUFATPACKAGINGWIZARD_H
