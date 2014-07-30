#ifndef UBUNTU_INTERNAL_UBUNTUFIRSTRUNWIZARD_H
#define UBUNTU_INTERNAL_UBUNTUFIRSTRUNWIZARD_H

#include <QWizard>
#include <QList>

class QLabel;
class QPushButton;
class QTreeWidget;
class QTextEdit;

namespace ProjectExplorer {
    class Kit;
}

namespace Ubuntu {
namespace Internal {

class UbuntuProcess;

class UbuntuFirstRunWizard : public QWizard
{
    Q_OBJECT
public:
    explicit UbuntuFirstRunWizard(QWidget *parent = 0);

signals:

public slots:

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
    void onCreateEmulatorsClicked ();
    void onProcMessage (const QString &text);
    void onProcFinished ( const QString &program, int code );

private:
    QPushButton *m_createButton;
    QTreeWidget *m_kitList;
    UbuntuProcess *m_proc;
    QTextEdit *m_procOutput;
    QList<ProjectExplorer::Kit *> m_kitsToCreate;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUFIRSTRUNWIZARD_H
