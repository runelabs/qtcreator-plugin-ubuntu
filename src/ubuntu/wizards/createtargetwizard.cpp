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
#include "createtargetwizard.h"
#include "ui_createtargetimagepage.h"
#include "ui_createtargetintropage.h"
#include "ui_createtargetnamepage.h"

#include <ubuntu/ubuntuconstants.h>

#include <coreplugin/icore.h>

#include <QJsonDocument>
#include <QMessageBox>
#include <QRadioButton>
#include <QFormLayout>
#include <QSpacerItem>

namespace Ubuntu {

namespace Constants {
    enum {
        INDEX_DATA = 0,
        INDEX_LOADING = 1,
        INDEX_ERROR = 2
    };

    const QMap<QString, QString> TARGET_ARCH_MAP({
        std::make_pair(QStringLiteral("i686"), QStringLiteral("i386")),
        std::make_pair(QStringLiteral("x86_64"), QStringLiteral("amd64")),
        std::make_pair(QStringLiteral("armv7l"), QStringLiteral("armhf")),
        std::make_pair(QStringLiteral("aarch64"), QStringLiteral("arm64")),
        std::make_pair(QStringLiteral("ppc"), QStringLiteral("powerpc")),
        std::make_pair(QStringLiteral("ppc64le"), QStringLiteral("ppc64el"))
    });

    const QString ARCH_FIELD(QStringLiteral("deviceArch"));
    const QString ALIAS_FIELD(QStringLiteral("imageAlias"));
}

namespace Internal {

CreateTargetWizard::CreateTargetWizard(QWidget *parent)
    : Utils::Wizard(parent)
{
    m_introPage = new CreateTargetIntroPage(this);
    addPage(m_introPage);

    m_imageSelectPage = new CreateTargetImagePage(this);
    m_imageSelectPage->setImageType(m_introPage->imageType());
    addPage(m_imageSelectPage);

    m_namePage = new CreateTargetNamePage(this);
    m_namePage->setImageType(m_introPage->imageType());
    addPage(m_namePage);

    connect(m_introPage, &CreateTargetIntroPage::imageTypeChanged,
            m_imageSelectPage, &CreateTargetImagePage::setImageType);

    connect(m_introPage, &CreateTargetIntroPage::imageTypeChanged,
            m_namePage, &CreateTargetNamePage::setImageType);

    setMinimumSize(800,400);
}


CreateTargetWizard::CreateTargetWizard(const QString &arch, const QString &framework, QWidget *parent)
    : Utils::Wizard(parent)
{
    m_introPage = nullptr;
    m_imageSelectPage = new CreateTargetImagePage(this);
    m_imageSelectPage->setFilter(arch, framework);
    addPage(m_imageSelectPage);

    m_namePage = new CreateTargetNamePage(this);
    addPage(m_namePage);

    setMinimumSize(800,400);
}

/**
 * @brief UbuntuCreateNewChrootDialog::getNewChrootParams
 * Opens a dialog that lets the user select a new chroot, returns false
 * if the user pressed cancel
 */
bool CreateTargetWizard::getNewTarget(UbuntuClickTool::Target *target, QWidget *parent)
{
    CreateTargetWizard dlg(parent ? parent : Core::ICore::mainWindow());
    return doSelectImage(dlg, target);
}

/**
 * @brief UbuntuCreateNewChrootDialog::getNewChrootParams
 * Opens the CreateTargetWizard but skips the intro page and applies the given filters
 */
bool CreateTargetWizard::getNewTarget(UbuntuClickTool::Target *target, const QString &arch, const QString &framework, QWidget *parent)
{

    CreateTargetWizard dlg(arch, framework, parent ? parent : Core::ICore::mainWindow());
    return doSelectImage(dlg, target);
}

bool CreateTargetWizard::doSelectImage (CreateTargetWizard &dlg, UbuntuClickTool::Target *target)
{
    if( dlg.exec() == QDialog::Accepted) {
        QString alias = dlg.m_imageSelectPage->selectedImageAlias();
        QString imageId = dlg.m_imageSelectPage->selectedImageId();
        if (alias.isEmpty() || imageId.isEmpty())
            return false;

        alias = alias.mid(0, alias.indexOf(QStringLiteral(" ")));

        if (!UbuntuClickTool::parseContainerName(alias, target))
            return false;

        target->containerName = dlg.m_namePage->chosenName();
        target->imageName = imageId;
        return true;
    }
    return false;
}

CreateTargetIntroPage::CreateTargetIntroPage(QWidget *parent)
    : Utils::WizardPage(parent)
    , ui(new Ui::CreateTargetIntroPage)

{
    ui->setupUi(this);

    m_imageTypeGroup = new QButtonGroup(this);
    m_imageTypeGroup->addButton(ui->desktopButton, CreateTargetWizard::DesktopImage);
    m_imageTypeGroup->addButton(ui->deviceButton, CreateTargetWizard::DeviceImage);
    m_imageTypeGroup->addButton(ui->allImagesButton, CreateTargetWizard::AllImages);
    m_imageTypeGroup->button(CreateTargetWizard::DesktopImage)->setChecked(true);

    connect(m_imageTypeGroup, SIGNAL(buttonClicked(int)),
            this, SLOT(buttonSelected(int)));

    setTitle(tr("Target creation"));
    setProperty(Utils::SHORT_TITLE_PROPERTY, tr("Intro"));
}

CreateTargetIntroPage::~CreateTargetIntroPage()
{
    delete ui;
}

CreateTargetWizard::ImageType CreateTargetIntroPage::imageType() const
{
    return static_cast<CreateTargetWizard::ImageType>(m_imageTypeGroup->checkedId());
}

void CreateTargetIntroPage::buttonSelected(const int id)
{
    emit imageTypeChanged(static_cast<CreateTargetWizard::ImageType>(id));
}

CreateTargetImagePage::CreateTargetImagePage(QWidget *parent) :
    Utils::WizardPage(parent),
    m_loader(nullptr),
    ui(new Ui::CreateTargetImagePage)
{
    ui->setupUi(this);
    ui->progressBar->setRange(0, 0);
    ui->stackedWidget->setCurrentIndex(Constants::INDEX_DATA);
    ui->treeWidgetImages->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    setTitle(tr("Please select the image:"));
    setProperty(Utils::SHORT_TITLE_PROPERTY, tr("Image"));

    registerField(Constants::ALIAS_FIELD, this, "imageAlias");
    registerField(Constants::ARCH_FIELD, this, "selectedDeviceArchitecture");
}

CreateTargetImagePage::~CreateTargetImagePage()
{
    delete ui;
}

void CreateTargetImagePage::setImageType(CreateTargetWizard::ImageType imageType)
{
    if (imageType == CreateTargetWizard::DesktopImage) {
        setFilter(UbuntuClickTool::hostArchitecture(), QString());
    } else if (imageType == CreateTargetWizard::DeviceImage){
        setFilter(QStringLiteral("(i386|aarch64|armhf)"), QString());
    } else {
        setFilter(QString(), QString());
    }
}

void CreateTargetImagePage::setFilter(const QString &arch, const QString &framework)
{
    QString fwFilter, archFilter = fwFilter = QStringLiteral("(.*)");
    if(!framework.isEmpty())
        fwFilter = framework;
    if(!arch.isEmpty())
        archFilter = arch;

    m_filter = QString::fromLatin1("^%1-%2").arg(fwFilter).arg(archFilter);
    load();
}

QString CreateTargetImagePage::selectedImageAlias() const
{
    QTreeWidgetItem * item = ui->treeWidgetImages->currentItem();
    if (!item)
        return QString();
    return item->data(0, Qt::UserRole+1).toString();
}

QString CreateTargetImagePage::selectedImageId() const
{
    QTreeWidgetItem * item = ui->treeWidgetImages->currentItem();
    if (!item)
        return QString();
    return item->data(0, Qt::UserRole).toString();

}

QString CreateTargetImagePage::selectedDeviceArchitecture() const
{
    QTreeWidgetItem * item = ui->treeWidgetImages->currentItem();
    if (!item)
        return QString();
    return item->text(1);
}

void CreateTargetImagePage::initializePage()
{
    load();
}

bool CreateTargetImagePage::validatePage()
{
    if (!ui->treeWidgetImages->currentItem()) {
        QMessageBox::warning(this, tr("No image selected"), tr("Please select a Image to continue."));
        return false;
    }
    return true;
}

void CreateTargetImagePage::load()
{
    if (m_loader) {
        m_loader->disconnect(this);
        if (m_loader->state() != QProcess::NotRunning) {
            m_loader->kill();
            m_loader->waitForFinished(1000);
        }
        m_loader->deleteLater();
        m_loader = nullptr;
    }

    ui->treeWidgetImages->clear();
    ui->stackedWidget->setCurrentIndex(Constants::INDEX_LOADING);

    m_loader = new QProcess(this);
    connect(m_loader, &QProcess::errorOccurred, this, &CreateTargetImagePage::loaderErrorOccurred);
    connect(m_loader, SIGNAL(finished(int)), this, SLOT(loaderFinished()));
    m_loader->setProgram(Constants::UBUNTU_TARGET_TOOL);
    m_loader->setArguments(QStringList{QStringLiteral("images")});

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    m_loader->setProcessEnvironment(env);

    m_loader->start();
}

void CreateTargetImagePage::loaderErrorOccurred(QProcess::ProcessError)
{
    ui->stackedWidget->setCurrentIndex(Constants::INDEX_ERROR);
    ui->errorLabel->setText(QStringLiteral("Error loading querying the images from the server"));
}

void CreateTargetImagePage::loaderFinished()
{
    if (m_loader->exitCode() != 0) {
        ui->stackedWidget->setCurrentIndex(Constants::INDEX_ERROR);
        ui->errorLabel->setText(QStringLiteral("Error loading querying the images from the server"));
        return;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(m_loader->readAllStandardOutput(), &err);
    if (err.error != QJsonParseError::NoError) {
        ui->stackedWidget->setCurrentIndex(Constants::INDEX_ERROR);
        ui->errorLabel->setText(QString::fromLatin1("Error loading the response from the server\n%1")
                                .arg(err.errorString()));
        return;
    }

    qDebug()<<"Filter is "<<m_filter;
    QRegularExpression expr(m_filter);
    QList<QVariant> data = doc.toVariant().toList();
    foreach (const QVariant &entry, data) {

        QVariantMap m = entry.toMap();
        QString alias = m.value(QStringLiteral("alias"), QStringLiteral("error")).toString();
        if(!expr.match(alias).hasMatch())
            continue;

        //check arch compat
        QString arch = m.value(QStringLiteral("arch"), QStringLiteral("error")).toString();
        if (!Constants::TARGET_ARCH_MAP.contains(arch))
            continue;

        if (!UbuntuClickTool::compatibleWithHostArchitecture(Constants::TARGET_ARCH_MAP[arch]))
            continue;

        UbuntuClickTool::Target target;
        if (!UbuntuClickTool::parseContainerName(alias, &target))
            continue;

        QTreeWidgetItem *item = new QTreeWidgetItem;
        item->setText(0,target.framework);
        item->setData(0, Qt::UserRole, m.value(QStringLiteral("fingerprint"), QStringLiteral("error")));
        item->setData(0, Qt::UserRole+1, alias);

        item->setText(1,target.architecture);
        item->setText(2,m.value(QStringLiteral("desc"), QStringLiteral("error")).toString());
        item->setText(3,m.value(QStringLiteral("size"), QStringLiteral("error")).toString());
        item->setText(4,m.value(QStringLiteral("uploadDate"), QStringLiteral("error")).toString());
        ui->treeWidgetImages->addTopLevelItem(item);
    }
    ui->stackedWidget->setCurrentIndex(Constants::INDEX_DATA);
    ui->treeWidgetImages->setCurrentItem(ui->treeWidgetImages->topLevelItem(0));
}

CreateTargetNamePage::CreateTargetNamePage(QWidget *parent) : Utils::WizardPage(parent),
    ui(new Ui::CreateTargetNamePage) ,
    m_imageType(-1)
{
    ui->setupUi(this);
    ui->lineEditName->setValidationFunction([](Utils::FancyLineEdit *edit, QString *errorMessage) {
        if (edit->text().isEmpty()) {
            if (errorMessage)
                *errorMessage = tr("Name can not be empty");
            return false;
        }

        QRegularExpression hostnameRegEx(QStringLiteral("^[A-Za-z0-9\\-_]+$"));
        if (!hostnameRegEx.match(edit->text()).hasMatch()){
            if (errorMessage)
                *errorMessage = tr("Name can only contain letters, numbers,dash and underscore");
            return false;
        }

        return true;
    });
    ui->lineEditName->setPlaceholderText(tr("Please select a name"));
    ui->lineEditName->triggerChanged();

    setTitle(tr("Please type a name:"));
    setProperty(Utils::SHORT_TITLE_PROPERTY, tr("Name"));
}

CreateTargetNamePage::~CreateTargetNamePage()
{
    delete ui;
}

void CreateTargetNamePage::setImageType(CreateTargetWizard::ImageType imageType)
{
    m_imageType = imageType;
}

QString CreateTargetNamePage::chosenName() const
{
    return ui->lineEditName->text();
}

void CreateTargetNamePage::initializePage()
{
    QString name;
    switch(m_imageType) {
        case CreateTargetWizard::DesktopImage:
            name = QStringLiteral("desktop");
            break;
        case CreateTargetWizard::DeviceImage:
            name = QString::fromLatin1("device-%1").arg(field(Constants::ARCH_FIELD).toString());
            break;
        default:
            name = QString::fromLatin1("builder-%1").arg(field(Constants::ARCH_FIELD).toString());
            break;
    }

    if (!name.isEmpty())
        ui->lineEditName->setText(name);
}

bool CreateTargetNamePage::validatePage()
{
    if (!ui->lineEditName->isValid()) {
        return false;
    }
    return true;
}

} // namespace Internal
} // namespace Ubuntu
