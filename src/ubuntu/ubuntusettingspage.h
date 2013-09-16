#ifndef UBUNTUSETTINGSPAGE_H
#define UBUNTUSETTINGSPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>
#include "ubuntuconstants.h"
#include "ubuntusettingswidget.h"
#include <QPointer>

namespace Ubuntu {
    namespace Internal {
        class UbuntuSettingsPage : public Core::IOptionsPage
        {
            Q_OBJECT

        public:
            explicit UbuntuSettingsPage();
            ~UbuntuSettingsPage();

            QWidget *createPage(QWidget *parent);
            void apply();
            void finish() { }

        protected:
            QPointer<UbuntuSettingsWidget> m_widget;
        };
    }
}

#endif // UBUNTUSETTINGSPAGE_H
