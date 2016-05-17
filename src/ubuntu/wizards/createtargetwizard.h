/*
 * Copyright 2014 Canonical Ltd.
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
 * Author: Benjamin Zeller <benjamin.zeller@canonical.com>
 */

#ifndef UBUNTU_INTERNAL_UBUNTUCREATENEWCHROOTDIALOG_H
#define UBUNTU_INTERNAL_UBUNTUCREATENEWCHROOTDIALOG_H


#include <ubuntu/ubuntuclicktool.h>

#include <utils/wizard.h>

#include <QPair>
#include <QProcess>
#include <QButtonGroup>

namespace Ubuntu {
namespace Internal {

namespace Ui {
    class CreateTargetImagePage;
    class CreateTargetIntroPage;
    class CreateTargetNamePage;
}

class CreateTargetIntroPage;
class CreateTargetImagePage;
class CreateTargetNamePage;

class CreateTargetWizard : public Utils::Wizard
{
    Q_OBJECT
public:

    enum ImageType {
        DesktopImage,
        DeviceImage
    };

    explicit CreateTargetWizard(QWidget *parent = 0);
    explicit CreateTargetWizard(const QString &arch, const QString &framework, QWidget *parent = 0);

    static bool getNewTarget(UbuntuClickTool::Target *target, QWidget *parent);
    static bool getNewTarget(UbuntuClickTool::Target *target, const QString &arch, const QString &framework, QWidget *parent);

private:
    static bool doSelectImage(CreateTargetWizard &dlg, UbuntuClickTool::Target *target);
private:
    CreateTargetIntroPage *m_introPage;
    CreateTargetImagePage *m_imageSelectPage;
    CreateTargetNamePage  *m_namePage;
};

class CreateTargetIntroPage : public QWizardPage
{
    Q_OBJECT
public:
    CreateTargetIntroPage (QWidget *parent = 0);
    ~CreateTargetIntroPage();

    CreateTargetWizard::ImageType imageType () const;

signals:
    void imageTypeChanged (CreateTargetWizard::ImageType imageType);

protected slots:
    void buttonSelected (const int id);

private:
    Ui::CreateTargetIntroPage *ui;
    QButtonGroup *m_imageTypeGroup;
};

class CreateTargetImagePage : public QWizardPage
{
    Q_OBJECT

public:
    explicit CreateTargetImagePage(QWidget *parent = 0);
    ~CreateTargetImagePage();

    void setImageType (CreateTargetWizard::ImageType imageType);
    void setFilter (const QString &arch, const QString &framework);

    QString selectedImageAlias () const;
    QString selectedImageId () const;
    QString chosenName () const;

    // QWizardPage interface
    virtual void initializePage() override;
    virtual bool validatePage() override;

protected:
    void load ();
    void loaderErrorOccurred(QProcess::ProcessError error);

protected slots:
    void loaderFinished();

private:
    QString m_filter;
    QProcess *m_loader;
    Ui::CreateTargetImagePage *ui;
};

class CreateTargetNamePage : public QWizardPage
{
    Q_OBJECT
public:
    CreateTargetNamePage(QWidget *parent = 0);
    ~CreateTargetNamePage();

    // QWizardPage interface
    virtual void initializePage() override;
    virtual bool validatePage() override;

    QString chosenName() const;
private:
    Ui::CreateTargetNamePage *ui;
};


} // namespace Internal
} // namespace Ubuntu
#endif // UBUNTU_INTERNAL_UBUNTUCREATENEWCHROOTDIALOG_H
