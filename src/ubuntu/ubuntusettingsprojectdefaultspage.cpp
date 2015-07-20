#include "ubuntusettingsprojectdefaultspage.h"
#include "ubuntuconstants.h"

#include <QWidget>

namespace Ubuntu {
namespace Internal {

UbuntuSettingsProjectDefaultsPage::UbuntuSettingsProjectDefaultsPage(QObject *parent)
    : Core::IOptionsPage(parent),
      m_ui(0)
{
    setId("B.ProjectDefaults");
    setDisplayName(tr("Project defaults"));
    setCategory("Ubuntu");
    setDisplayCategory(QLatin1String("Ubuntu"));
    setCategoryIcon(QLatin1String(Ubuntu::Constants::UBUNTU_SETTINGS_ICON));

    m_settings = new QSettings(QLatin1String(Constants::SETTINGS_COMPANY),QLatin1String(Constants::SETTINGS_PRODUCT), this);
}

UbuntuSettingsProjectDefaultsPage::~UbuntuSettingsProjectDefaultsPage()
{
    if (m_ui)
        delete m_ui;
}

QWidget *UbuntuSettingsProjectDefaultsPage::widget()
{
    if (!m_widget) {
        m_widget = new QWidget();
        m_ui = new ::Ui::UbuntuSettingsDefaultPage;
        m_ui->setupUi(m_widget);

        m_settings->sync();
        m_settings->beginGroup(QLatin1String(Constants::SETTINGS_GROUP_PROJECT_DEFAULTS));
        m_ui->checkBoxDebugHelper->setChecked(m_settings->value(QLatin1String(Constants::SETTINGS_KEY_ENABLE_DEBUG_HELPER_DEFAULT),true).toBool());
        m_ui->checkBoxOverrideApps->setChecked(m_settings->value(QLatin1String(Constants::SETTINGS_KEY_OVERRIDE_APPS_BY_DEFAULT),false).toBool());
        m_ui->checkBoxUninstallApps->setChecked(m_settings->value(QLatin1String(Constants::SETTINGS_KEY_UNINSTALL_APPS_FROM_DEVICE_DEFAULT),true).toBool());
        m_ui->checkBoxClickErrors->setChecked(m_settings->value(QLatin1String(Constants::SETTINGS_KEY_TREAT_REVIEW_ERRORS_AS_WARNINGS),false).toBool());
        m_settings->endGroup();


    }

    return m_widget;
}

void UbuntuSettingsProjectDefaultsPage::apply()
{
    m_settings->beginGroup(QLatin1String(Constants::SETTINGS_GROUP_PROJECT_DEFAULTS));
    m_settings->setValue(QLatin1String(Constants::SETTINGS_KEY_ENABLE_DEBUG_HELPER_DEFAULT),
                         m_ui->checkBoxDebugHelper->checkState() == Qt::Checked);

    m_settings->setValue(QLatin1String(Constants::SETTINGS_KEY_OVERRIDE_APPS_BY_DEFAULT),
                         m_ui->checkBoxOverrideApps->checkState() == Qt::Checked);

    m_settings->setValue(QLatin1String(Constants::SETTINGS_KEY_UNINSTALL_APPS_FROM_DEVICE_DEFAULT),
                         m_ui->checkBoxUninstallApps->checkState() == Qt::Checked);

    m_settings->setValue(QLatin1String(Constants::SETTINGS_KEY_TREAT_REVIEW_ERRORS_AS_WARNINGS),
                         m_ui->checkBoxClickErrors->checkState() == Qt::Checked);
    m_settings->endGroup();

    m_settings->sync();
}

void UbuntuSettingsProjectDefaultsPage::finish()
{
    if (m_widget) {
        delete m_widget;
        delete m_ui;
        m_ui = 0;
    }
}

} // namespace Internal
} // namespace Ubuntu
