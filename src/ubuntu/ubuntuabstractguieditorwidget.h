#ifndef UBUNTUABSTRACTGUIEDITORWIDGET_H
#define UBUNTUABSTRACTGUIEDITORWIDGET_H

#include "ubuntuclickmanifest.h"

#include <texteditor/basetexteditor.h>
#include <texteditor/plaintexteditor.h>

#include <QScrollArea>
#include "ui_ubuntumanifesteditor.h"

class QStackedWidget;

namespace ProjectExplorer {class Project;}

namespace Ubuntu {
namespace Internal {

class UbuntuAbstractGuiEditor;
class UbuntuAbstractGuiEditorWidget;
class UbuntuClickManifest;

class UbuntuManifestTextEditorWidget : public TextEditor::PlainTextEditorWidget
{
public:
    UbuntuManifestTextEditorWidget(QString mimeType, UbuntuAbstractGuiEditorWidget *parent = 0);
protected:
    UbuntuAbstractGuiEditorWidget *m_parent;
};

class UbuntuAbstractGuiEditorWidget : public QScrollArea
{
    Q_OBJECT
public:
    enum EditorPage {
        General = 0,
        Source = 1
    };

    explicit UbuntuAbstractGuiEditorWidget(const QString &mimeType);
    ~UbuntuAbstractGuiEditorWidget();

    virtual bool open(QString *errorString, const QString &fileName, const QString &realFileName);
    virtual bool isModified() const;

    EditorPage activePage() const;
    bool setActivePage(EditorPage page);

    bool preSave();

    TextEditor::PlainTextEditorWidget *textEditorWidget() const;

protected slots:
    void setDirty ();
    void createUI ();

protected:
    virtual bool syncToWidgets () = 0;
    virtual void syncToSource  () = 0;
    virtual QWidget *createMainWidget () = 0;

    void updateInfoBar(const QString &errorMessage);

signals:
    void uiEditorChanged();

protected:
    QStackedWidget *m_widgetStack;
    TextEditor::PlainTextEditorWidget *m_sourceEditor;

    bool m_dirty;
};

ProjectExplorer::Project *ubuntuProject(const QString &file);

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTUABSTRACTGUIEDITORWIDGET_H
