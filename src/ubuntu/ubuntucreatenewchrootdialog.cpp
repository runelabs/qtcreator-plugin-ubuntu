#include "ubuntucreatenewchrootdialog.h"
#include "ui_ubuntucreatenewchrootdialog.h"

#include "ubuntuconstants.h"

namespace Ubuntu {

namespace Constants {
    const char* UBUNTU_CLICK_SUPPORTED_ARCHS[]  = {"armhf","i386","amd64","\0"};
    const char* UBUNTU_CLICK_SUPPORTED_SERIES[] = {"saucy","trusty","\0"};
}

namespace Internal {

UbuntuCreateNewChrootDialog::UbuntuCreateNewChrootDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UbuntuCreateNewChrootDialog)
{
    ui->setupUi(this);

    //add supported architectures
    for(int i = 0; Constants::UBUNTU_CLICK_SUPPORTED_ARCHS[i][0] != '\0' ;i++) {
        ui->comboBoxArch->addItem(QLatin1String(Constants::UBUNTU_CLICK_SUPPORTED_ARCHS[i]));
    }

    //add supported series
    for(int i = 0; Constants::UBUNTU_CLICK_SUPPORTED_SERIES[i][0] != '\0' ;i++) {
        ui->comboBoxSeries->addItem(QLatin1String(Constants::UBUNTU_CLICK_SUPPORTED_SERIES[i]));
    }
}

UbuntuCreateNewChrootDialog::~UbuntuCreateNewChrootDialog()
{
    delete ui;
}

/**
 * @brief UbuntuCreateNewChrootDialog::getNewChrootParams
 * Opens a dialog that lets the user select a new chroot, returns a pair of
 * empty strings if the user pressed cancel
 */
QPair<QString, QString> UbuntuCreateNewChrootDialog::getNewChrootParams()
{
    UbuntuCreateNewChrootDialog dlg;
    if( dlg.exec() == QDialog::Accepted)
        return qMakePair(dlg.ui->comboBoxArch->currentText(),dlg.ui->comboBoxSeries->currentText());

    return QPair<QString,QString>();
}

} // namespace Internal
} // namespace Ubuntu
