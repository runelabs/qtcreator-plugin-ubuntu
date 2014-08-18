#ifndef UBUNTU_INTERNAL_UBUNTUAPPARMOREDITOR_H
#define UBUNTU_INTERNAL_UBUNTUAPPARMOREDITOR_H

#include "ubuntuabstractguieditor.h"
#include "ubuntuabstractguieditorwidget.h"
#include "ui_ubuntuapparmoreditor.h"

#include <QSharedPointer>

namespace Ubuntu {
namespace Internal {

class UbuntuClickManifest;
class UbuntuApparmorEditorWidget;

class UbuntuApparmorEditor : public UbuntuAbstractGuiEditor
{
    Q_OBJECT
public:
    UbuntuApparmorEditor();
    ~UbuntuApparmorEditor();

protected:
    // UbuntuAbstractGuiEditor interface
    virtual UbuntuAbstractGuiEditorWidget *createGuiEditor();

private:
    UbuntuApparmorEditorWidget *m_editorWidget;
};

class UbuntuApparmorEditorWidget : public UbuntuAbstractGuiEditorWidget
{
    Q_OBJECT
public:
    UbuntuApparmorEditorWidget();
    ~UbuntuApparmorEditorWidget();

    // UbuntuAbstractGuiEditorWidget interface
public:
    virtual bool open(QString *errorString, const QString &fileName, const QString &realFileName);

protected:
    virtual bool syncToWidgets();
    bool syncToWidgets(UbuntuClickManifest *source);
    virtual void syncToSource();
    virtual QWidget *createMainWidget();

protected slots:
    void on_pushButton_addpolicy_clicked();    
    void on_listWidget_customContextMenuRequested(const QPoint &p);

private:
    QSharedPointer<UbuntuClickManifest> m_apparmor;
    Ui::UbuntuAppArmorEditor *m_ui;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUAPPARMOREDITOR_H
