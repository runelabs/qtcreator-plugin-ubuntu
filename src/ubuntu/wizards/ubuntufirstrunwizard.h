#ifndef UBUNTU_INTERNAL_UBUNTUFIRSTRUNWIZARD_H
#define UBUNTU_INTERNAL_UBUNTUFIRSTRUNWIZARD_H

#include <utils/wizard.h>
#include <QList>

class QLabel;
class QPushButton;
class QTreeWidget;
class QCheckBox;

namespace ProjectExplorer {
    class Kit;
}

namespace Ubuntu {
namespace Internal {

class UbuntuProcess;

class UbuntuFirstRunWizard : public Utils::Wizard
{
    Q_OBJECT
public:
    explicit UbuntuFirstRunWizard(QWidget *parent = 0);

signals:

public slots:

};

class UbuntuIntroductionWizardPage : public QWizardPage
{
    Q_OBJECT
public:
    UbuntuIntroductionWizardPage (QWidget *parent = 0);

    // QWizardPage interface
    virtual void initializePage();
    virtual bool isComplete() const;
};


class UbuntuSetupChrootWizardPage : public QWizardPage
{
    Q_OBJECT
public:
    UbuntuSetupChrootWizardPage (QWidget *parent = 0);

    // QWizardPage interface
    virtual void initializePage();
    virtual bool isComplete() const;

protected slots:
    void onCreateKitButtonClicked ();

private:
    QLabel *m_kitExistsLabel;
    QTreeWidget *m_kitList;
    QPushButton *m_createKitButton;
    bool m_complete;
};

class UbuntuSetupEmulatorWizardPage : public QWizardPage
{
    Q_OBJECT
public:
    UbuntuSetupEmulatorWizardPage (QWidget *parent = 0);

    // QWizardPage interface
    virtual void initializePage();
    virtual bool isComplete() const;

protected slots:
    void updateDevicesList ();

private:
    QCheckBox *m_createEmulatorCheckBox;
    QTreeWidget *m_devicesList;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUFIRSTRUNWIZARD_H
