#include "ubuntusettingsprojectdefaultspage.h"
#include "ubuntuconstants.h"
#include "settings.h"

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

        Settings::ProjectDefaults def = Settings::projectDefaults();

        m_ui->checkBoxDebugHelper->setChecked(def.enableDebugHelper);
        m_ui->checkBoxOverrideApps->setChecked(def.overrideAppsByDefault);
        m_ui->checkBoxUninstallApps->setChecked(def.uninstallAppsByDefault);
        m_ui->checkBoxClickErrors->setChecked(def.reviewErrorsAsWarnings);
    }

    return m_widget;
}

void UbuntuSettingsProjectDefaultsPage::apply()
{
    Settings::ProjectDefaults newDef;
    newDef.enableDebugHelper = m_ui->checkBoxDebugHelper->checkState() == Qt::Checked;
    newDef.overrideAppsByDefault = m_ui->checkBoxOverrideApps->checkState() == Qt::Checked;
    newDef.reviewErrorsAsWarnings = m_ui->checkBoxClickErrors->checkState() == Qt::Checked;
    newDef.uninstallAppsByDefault = m_ui->checkBoxUninstallApps->checkState() == Qt::Checked;
    Settings::setProjectDefaults(newDef);
    Settings::flushSettings();
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
