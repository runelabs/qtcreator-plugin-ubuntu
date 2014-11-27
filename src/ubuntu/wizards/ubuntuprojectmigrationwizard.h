#ifndef UBUNTU_INTERNAL_UBUNTUPROJECTMIGRATIONWIZARD_H
#define UBUNTU_INTERNAL_UBUNTUPROJECTMIGRATIONWIZARD_H

#include <utils/wizard.h>
#include <QStringList>

namespace QmakeProjectManager{
    class QmakeProject;
}

namespace Ubuntu {
namespace Internal {

class UbuntuProjectMigrationWizard : public Utils::Wizard
{
    Q_OBJECT
public:
    explicit UbuntuProjectMigrationWizard(QmakeProjectManager::QmakeProject *project, QWidget *parent = 0);

    QmakeProjectManager::QmakeProject *project() const;
    QStringList relevantTargets () const;

signals:

public slots:

private:
    QmakeProjectManager::QmakeProject *m_project;

};

class UbuntuSelectSubProjectsPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit UbuntuSelectSubProjectsPage(QWidget *parent = 0);

    // QWizardPage interface
public:
    virtual void initializePage();
    virtual bool isComplete() const;
};

class UbuntuProjectDetailsPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit UbuntuProjectDetailsPage(QWidget *parent = 0);

    // QWizardPage interface
public:
    virtual void initializePage();
    virtual bool isComplete() const;

private:
    bool m_initialized;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUPROJECTMIGRATIONWIZARD_H
