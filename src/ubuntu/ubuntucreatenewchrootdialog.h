#ifndef UBUNTU_INTERNAL_UBUNTUCREATENEWCHROOTDIALOG_H
#define UBUNTU_INTERNAL_UBUNTUCREATENEWCHROOTDIALOG_H

#include <QDialog>
#include <QPair>

namespace Ubuntu {
namespace Internal {

namespace Ui {
    class UbuntuCreateNewChrootDialog;
}

class UbuntuCreateNewChrootDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UbuntuCreateNewChrootDialog(QWidget *parent = 0);
    ~UbuntuCreateNewChrootDialog();

    static QPair<QString,QString> getNewChrootParams ();

private:
    Ui::UbuntuCreateNewChrootDialog *ui;
};


} // namespace Internal
} // namespace Ubuntu
#endif // UBUNTU_INTERNAL_UBUNTUCREATENEWCHROOTDIALOG_H
