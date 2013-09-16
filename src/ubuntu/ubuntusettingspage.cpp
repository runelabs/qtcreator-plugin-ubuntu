#include "ubuntusettingspage.h"

using namespace Ubuntu::Internal;
using namespace Ubuntu;

UbuntuSettingsPage::UbuntuSettingsPage() :
    m_widget(0)
{
    setId("A.General");
    setDisplayName(tr("General"));
    setCategory("Ubuntu");
    setDisplayCategory(QLatin1String("Ubuntu"));
    setCategoryIcon(QLatin1String(Constants::UBUNTU_SETTINGS_ICON));
}

UbuntuSettingsPage::~UbuntuSettingsPage()
{
}

QWidget *UbuntuSettingsPage::createPage(QWidget *parent)
{
    m_widget = new UbuntuSettingsWidget(parent);
    return m_widget;
}

void UbuntuSettingsPage::apply()
{
    if (!m_widget) // page was never shown
        return;

    m_widget->apply();
}
