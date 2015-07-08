#ifndef UBUNTU_INTERNAL_UBUNTUSETTINGSPROJECTDEFAULTS_H
#define UBUNTU_INTERNAL_UBUNTUSETTINGSPROJECTDEFAULTS_H

#include "ui_ubuntusettingsprojectdefaultspage.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <QPointer>

class QWidget;
class QSettings;

namespace Ubuntu {
namespace Internal {

class UbuntuSettingsProjectDefaultsPage: public Core::IOptionsPage
{
    Q_OBJECT
public:
    explicit UbuntuSettingsProjectDefaultsPage(QObject *parent = 0);
    ~UbuntuSettingsProjectDefaultsPage();

    QWidget *widget( ) override;
    void apply() override;
    void finish() override;

signals:

public slots:

private:
    QSettings *m_settings;
    QPointer<QWidget> m_widget;
    ::Ui::UbuntuSettingsDefaultPage *m_ui;

};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUSETTINGSPROJECTDEFAULTS_H
