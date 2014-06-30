/*
 * Copyright 2013 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.1.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Juhapekka Piiroinen <juhapekka.piiroinen@canonical.com>
 */

#ifndef UBUNTUPROJECTAPPLICATIONWIZARD_H
#define UBUNTUPROJECTAPPLICATIONWIZARD_H

#include <projectexplorer/baseprojectwizarddialog.h>
#include <projectexplorer/customwizard/customwizard.h>
#include <projectexplorer/targetsetuppage.h>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

namespace Utils {
class PathChooser;
}

namespace Ubuntu {
namespace Internal {

class UbuntuProjectApplicationWizard : public ProjectExplorer::CustomProjectWizard
{
    Q_OBJECT

public:

    enum ProjectType{
        UbuntuProject, //handles Qml, Html basic projects
        CMakeProject,
        GoProject
    };

    UbuntuProjectApplicationWizard(ProjectType type);

private:
    QWizard *createWizardDialog(QWidget *parent,
                                const Core::WizardDialogParameters &wizardDialogParameters) const override;
    bool postGenerateFiles(const QWizard *, const Core::GeneratedFiles &l, QString *errorMessage) override;

private:
    Core::FeatureSet requiredFeatures() const;
    ProjectType m_type;
};

class UbuntuProjectApplicationWizardDialog : public ProjectExplorer::BaseProjectWizardDialog
{
    Q_OBJECT
public:
    explicit UbuntuProjectApplicationWizardDialog(QWidget *parent,
                                                  UbuntuProjectApplicationWizard::ProjectType type,
                                                  const Core::WizardDialogParameters &parameters);
    virtual ~UbuntuProjectApplicationWizardDialog();

    void addTargetSetupPage(int id = -1);

    QList<Core::Id> selectedKits() const;
    bool writeUserFile(const QString &projectFileName) const;
private slots:
    void generateProfileName(const QString &projectName, const QString &path);
private:
    ProjectExplorer::TargetSetupPage *m_targetSetupPage;
    void init();
    UbuntuProjectApplicationWizard::ProjectType m_type;
};

template <class Wizard,UbuntuProjectApplicationWizard::ProjectType type> class UbuntuWizardFactory : public ProjectExplorer::ICustomWizardFactory
{
public:
    UbuntuWizardFactory(const QString &klass, Core::IWizard::WizardKind kind) : ICustomWizardFactory(klass, kind) { }
    UbuntuWizardFactory(Core::IWizard::WizardKind kind) : ICustomWizardFactory(QString(), kind) { }
    ProjectExplorer::CustomWizard *create() const { return new Wizard(type); }
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTUPROJECTAPPLICATIONWIZARD_H
