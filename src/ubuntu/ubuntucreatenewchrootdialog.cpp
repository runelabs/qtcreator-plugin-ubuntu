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
#include "ubuntucreatenewchrootdialog.h"
#include "ubuntuclicktool.h"
#include "ui_ubuntucreatenewchrootdialog.h"

#include "ubuntuconstants.h"

#include <coreplugin/icore.h>

#include <QJsonDocument>
#include <QMessageBox>

namespace Ubuntu {

namespace Constants {
    enum {
        INDEX_DATA = 0,
        INDEX_LOADING = 1,
        INDEX_ERROR = 2
    };
}

namespace Internal {

UbuntuCreateNewChrootDialog::UbuntuCreateNewChrootDialog(const QString &arch, const QString &framework,  QWidget *parent) :
    QDialog(parent),
    m_loader(nullptr),
    ui(new Ui::UbuntuCreateNewChrootDialog)
{
    ui->setupUi(this);
    ui->progressBar->setRange(0, 0);
    ui->stackedWidget->setCurrentIndex(Constants::INDEX_DATA);

    QString fwFilter, archFilter = fwFilter = QStringLiteral("(.*)");
    if(!framework.isEmpty())
        fwFilter = framework;
    if(!arch.isEmpty())
        archFilter = arch;

    m_filter = QString::fromLatin1("^%1-%2").arg(fwFilter).arg(archFilter);
    load();

    ui->lineEditName->setValidationFunction([](Utils::FancyLineEdit *edit, QString *errorMessage) {
        if (edit->text().isEmpty()) {
            if (errorMessage)
                *errorMessage = tr("Name can not be empty");
            return false;
        }
        return true;
    });
    ui->lineEditName->setPlaceholderText(tr("Please select a name"));
    ui->lineEditName->triggerChanged();
}

UbuntuCreateNewChrootDialog::~UbuntuCreateNewChrootDialog()
{
    delete ui;
}

/**
 * @brief UbuntuCreateNewChrootDialog::getNewChrootParams
 * Opens a dialog that lets the user select a new chroot, returns false
 * if the user pressed cancel
 */
bool UbuntuCreateNewChrootDialog::getNewChrootTarget(UbuntuClickTool::Target *target, const QString &arch, const QString &framework, QWidget *parent)
{
    UbuntuCreateNewChrootDialog dlg(arch, framework, parent ? parent : Core::ICore::mainWindow());
    if( dlg.exec() == QDialog::Accepted) {
        QTreeWidgetItem * item = dlg.ui->treeWidgetImages->currentItem();
        if (!item)
            return false;

        QString alias = item->text(0);
        alias = alias.mid(0, alias.indexOf(QStringLiteral(" ")));

        if (!UbuntuClickTool::parseContainerName(alias, target))
            return false;

        target->containerName = dlg.ui->lineEditName->text();
        target->imageName = item->text(1);
        return true;
    }
    return false;
}

void UbuntuCreateNewChrootDialog::accept()
{
    if (!ui->treeWidgetImages->currentItem()) {
        QMessageBox::warning(this, tr("No image selected"), tr("Please select a Image to continue."));
        return;
    }
    if (!ui->lineEditName->isValid()) {
        QMessageBox::warning(this, tr("No target name given"), tr("Invalid value for name given."));
        return;
    }
    QDialog::accept();
}

void UbuntuCreateNewChrootDialog::load()
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

    ui->stackedWidget->setCurrentIndex(Constants::INDEX_LOADING);

    m_loader = new QProcess(this);
    connect(m_loader, &QProcess::errorOccurred, this, &UbuntuCreateNewChrootDialog::loaderErrorOccurred);
    connect(m_loader, SIGNAL(finished(int)), this, SLOT(loaderFinished()));
    m_loader->setProgram(Constants::UBUNTU_TARGET_TOOL);
    m_loader->setArguments(QStringList{QStringLiteral("images")});

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    m_loader->setProcessEnvironment(env);

    m_loader->start();

}

void UbuntuCreateNewChrootDialog::loaderErrorOccurred(QProcess::ProcessError)
{
    ui->stackedWidget->setCurrentIndex(Constants::INDEX_ERROR);
    ui->errorLabel->setText(QStringLiteral("Error loading querying the images from the server"));
}

void UbuntuCreateNewChrootDialog::loaderFinished()
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
        qDebug()<<m;
        QString alias = m.value(QStringLiteral("alias"), QStringLiteral("error")).toString();
        if(!expr.match(alias).hasMatch())
            continue;

        //check arch compat

        QTreeWidgetItem *item = new QTreeWidgetItem;
        item->setText(0,alias);
        item->setText(1,m.value(QStringLiteral("fingerprint"), QStringLiteral("error")).toString());
        item->setText(2,m.value(QStringLiteral("desc"), QStringLiteral("error")).toString());
        item->setText(3,m.value(QStringLiteral("arch"), QStringLiteral("error")).toString());
        item->setText(4,m.value(QStringLiteral("size"), QStringLiteral("error")).toString());
        item->setText(5,m.value(QStringLiteral("uploadDate"), QStringLiteral("error")).toString());
        ui->treeWidgetImages->addTopLevelItem(item);
    }
    ui->stackedWidget->setCurrentIndex(Constants::INDEX_DATA);
}

} // namespace Internal
} // namespace Ubuntu
