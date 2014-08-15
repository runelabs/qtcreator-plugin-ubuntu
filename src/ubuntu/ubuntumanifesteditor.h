#ifndef UBUNTU_INTERNAL_UBUNTUMANIFESTEDITOR_H
#define UBUNTU_INTERNAL_UBUNTUMANIFESTEDITOR_H

#include <coreplugin/editormanager/ieditor.h>
#include <texteditor/basetexteditor.h>

class QToolBar;

namespace Ubuntu {
namespace Internal {

class UbuntuManifestEditorWidget;

class UbuntuManifestEditor : public Core::IEditor
{
    Q_OBJECT
public:
    UbuntuManifestEditor();

    bool open(QString *errorString, const QString &fileName, const QString &realFileName);
    QWidget *toolBar();
    UbuntuManifestEditorWidget *editorWidget() const;
    Core::IDocument *document();
    TextEditor::BaseTextEditorWidget *textEditor() const;

    int currentLine() const;
    int currentColumn() const;
    void gotoLine(int line, int column = 0) { textEditor()->gotoLine(line, column); }

private:
    void syncCurrentAction ();

private slots:
    void changeEditorPage(QAction *action);

private:
    QString m_displayName;
    QToolBar *m_toolBar;
    QActionGroup *m_actionGroup;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUMANIFESTEDITOR_H
