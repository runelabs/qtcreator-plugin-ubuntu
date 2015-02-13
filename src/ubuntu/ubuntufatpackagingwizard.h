#ifndef UBUNTU_INTERNAL_UBUNTUFATPACKAGINGWIZARD_H
#define UBUNTU_INTERNAL_UBUNTUFATPACKAGINGWIZARD_H

#include <utils/wizard.h>

#include <QList>

class QTreeWidget;
class QTreeWidgetItem;
namespace ProjectExplorer{ class BuildConfiguration; }

namespace Ubuntu {
namespace Internal {

class UbuntuFatPackagingWizard : public Utils::Wizard
{
    Q_OBJECT
public:
    explicit UbuntuFatPackagingWizard(QList<ProjectExplorer::BuildConfiguration *> suspects, QWidget *parent = 0);

signals:

public slots:

};

class UbuntuFatPackagingIntroPage : public QWizardPage
{
    Q_OBJECT
public:
    UbuntuFatPackagingIntroPage (QWidget *parent = 0);

    // QWizardPage interface
    virtual void initializePage();
    virtual bool isComplete() const;
};

class UbuntuChooseTargetPage : public QWizardPage
{
    Q_OBJECT
    Q_PROPERTY(QList<int> selectedSuspects READ selectedSuspects NOTIFY selectedSuspectsChanged)
public:
    UbuntuChooseTargetPage (QList<ProjectExplorer::BuildConfiguration *> suspects, QWidget *parent = 0);

    // QWizardPage interface
    virtual void initializePage();
    virtual bool isComplete() const;

    QList<int> selectedSuspects() const;

signals:
    void selectedSuspectsChanged(const QList<int> &arg);

private:
    void itemChanged (QTreeWidgetItem * item, int column);

private:
    QTreeWidget *m_targetView;
    QList<int> m_selectedSuspects;
    QList<ProjectExplorer::BuildConfiguration *> m_suspects;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUFATPACKAGINGWIZARD_H
