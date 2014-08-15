#ifndef UBUNTU_INTERNAL_UBUNTUMANIFESTEDITORWIDGET_H
#define UBUNTU_INTERNAL_UBUNTUMANIFESTEDITORWIDGET_H

#include "ubuntuclickmanifest.h"

#include <texteditor/basetexteditor.h>
#include <texteditor/plaintexteditor.h>

#include <QScrollArea>
#include "ui_ubuntumanifesteditor.h"

class QStackedWidget;

namespace Ubuntu {
namespace Internal {

class UbuntuManifestEditor;
class UbuntuManifestEditorWidget;
class UbuntuClickManifest;

class UbuntuManifestTextEditorWidget : public TextEditor::PlainTextEditorWidget
{
public:
    UbuntuManifestTextEditorWidget(UbuntuManifestEditorWidget *parent = 0);
protected:
    UbuntuManifestEditorWidget *m_parent;
};

class UbuntuManifestEditorWidget : public QScrollArea
{
    Q_OBJECT
public:
    enum EditorPage {
        General = 0,
        Source = 1
    };

    explicit UbuntuManifestEditorWidget(UbuntuManifestEditor *editor);
    ~UbuntuManifestEditorWidget();

    bool open(QString *errorString, const QString &fileName, const QString &realFileName);

    bool isModified() const;

    EditorPage activePage() const;
    bool setActivePage(EditorPage page);

    bool preSave();

    Core::IEditor *editor() const;
    TextEditor::PlainTextEditorWidget *textEditorWidget() const;

protected slots:
    void setDirty ();

protected:
    bool syncToWidgets ();
    bool syncToWidgets (UbuntuClickManifest *source);
    void syncToSource  ();
    void updateFrameworkList ();

    void updateInfoBar(const QString &errorMessage);
private:
    QWidget *createHookWidget (const UbuntuClickManifest::Hook &hook);

signals:
    void uiEditorChanged();

private:
    Ui::UbuntuManifestEditor *m_ui;
    UbuntuManifestEditor *m_editor;
    QStackedWidget *m_widgetStack;
    TextEditor::PlainTextEditorWidget *m_sourceEditor;

    QSharedPointer<UbuntuClickManifest> m_manifest;
    bool m_dirty;

};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUMANIFESTEDITORWIDGET_H
