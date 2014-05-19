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

#include <coreplugin/basefilewizard.h>
#include <projectexplorer/baseprojectwizarddialog.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmlprojectmanager/qmlproject.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/targetsetuppage.h>

#include <QJsonObject>
#include "ubuntuprojectapp.h"

namespace Ubuntu {
namespace Internal {

class UbuntuProjectApplicationWizardDialog : public ProjectExplorer::BaseProjectWizardDialog
{
    Q_OBJECT
public:
    explicit UbuntuProjectApplicationWizardDialog(QWidget *parent,
                                                  const Core::WizardDialogParameters &parameters,
                                                  const QString &projectType);

    int addTargetSetupPage(int id);
    bool writeUserFile(const QString &cmakeFileName) const;

private slots:
    void on_projectParametersChanged(const QString &projectName, const QString &path);


private:
    ProjectExplorer::TargetSetupPage *m_targetSetupPage;
    QString m_projectType;
};

class UbuntuProjectApplicationWizard : public Core::BaseFileWizard
{
    Q_OBJECT

public:
    UbuntuProjectApplicationWizard(QJsonObject);
    virtual ~UbuntuProjectApplicationWizard();
    virtual Core::FeatureSet requiredFeatures() const;

    static QByteArray getProjectTypesJSON();

    static QString templatesPath(QString fileName);

protected:
    virtual QWizard *createWizardDialog(QWidget *parent,
                                        const Core::WizardDialogParameters &wizardDialogParameters) const;

    virtual Core::GeneratedFiles generateFiles(const QWizard *w,
                                               QString *errorMessage) const;

    virtual bool postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage);

private:
    UbuntuProjectApp* m_app;
    QString           m_projectType;
};

} // Internal
} // Ubuntu


#endif // UBUNTUPROJECTAPPLICATIONWIZARD_H
