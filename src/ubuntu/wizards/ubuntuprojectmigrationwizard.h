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
    QmakeProjectManager::QmakeProject *project() const;
    QStringList relevantTargets () const;

    QStringList selectedTargets () const;
    QString maintainer () const;
    QString domain     () const;
    QString framework  () const;

    static void doMigrateProject (QmakeProjectManager::QmakeProject *project, QWidget *parent = 0);

signals:

public slots:

private:
    explicit UbuntuProjectMigrationWizard(QmakeProjectManager::QmakeProject *project, QWidget *parent = 0);
    QmakeProjectManager::QmakeProject *m_project;

};

class UbuntuProjectMigrationIntroPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit UbuntuProjectMigrationIntroPage(QWidget *parent = 0);
};

class UbuntuSelectSubProjectsPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit UbuntuSelectSubProjectsPage(QWidget *parent = 0);

    // QWizardPage interface
public:
    virtual void initializePage() override;
    virtual bool isComplete() const override;
};

class UbuntuProjectDetailsPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit UbuntuProjectDetailsPage(QWidget *parent = 0);

    // QWizardPage interface
public:
    virtual void initializePage() override;
    virtual bool isComplete() const override;

private:
    bool m_initialized;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUPROJECTMIGRATIONWIZARD_H
