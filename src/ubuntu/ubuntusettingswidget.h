#ifndef UBUNTUSETTINGSWIDGET_H
#define UBUNTUSETTINGSWIDGET_H

#include <QWidget>
#include <QSettings>

namespace Ui {
class UbuntuSettingsWidget;
}

class UbuntuSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit UbuntuSettingsWidget(QWidget *parent = 0);
    ~UbuntuSettingsWidget();

    void apply();

private:
    Ui::UbuntuSettingsWidget *ui;
    QSettings* m_settings;
};

#endif // UBUNTUSETTINGSWIDGET_H
