#include "ubuntumanifesteditor.h"
#include "ubuntumanifesteditorwidget.h"
#include "ubuntuconstants.h"

#include <texteditor/texteditorconstants.h>

#include <QActionGroup>
#include <QToolBar>
#include <QTextBlock>

namespace Ubuntu {
namespace Internal {

UbuntuManifestEditor::UbuntuManifestEditor() : Core::IEditor(), m_toolBar(0)
{
    setId(Constants::UBUNTU_MANIFEST_EDITOR_ID);

    m_widget = new UbuntuManifestEditorWidget(this);

    m_toolBar = new QToolBar(m_widget.data());
    m_actionGroup = new QActionGroup(m_widget.data());
    connect(m_actionGroup, SIGNAL(triggered(QAction*)), this, SLOT(changeEditorPage(QAction*)));

    QAction *generalAction = m_toolBar->addAction(tr("General"));
    generalAction->setData(UbuntuManifestEditorWidget::General);
    generalAction->setCheckable(true);
    m_actionGroup->addAction(generalAction);

    QAction *sourceAction = m_toolBar->addAction(tr("JSON Source"));
    sourceAction->setData(UbuntuManifestEditorWidget::Source);
    sourceAction->setCheckable(true);
    m_actionGroup->addAction(sourceAction);

    generalAction->setChecked(true);

    setContext(Core::Context(Constants::UBUNTU_MANIFEST_EDITOR_CONTEXT));
    setWidget(editorWidget());
}

bool UbuntuManifestEditor::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    return editorWidget()->open(errorString,fileName,realFileName);
}

QWidget *UbuntuManifestEditor::toolBar()
{
    return m_toolBar;
}

UbuntuManifestEditorWidget *UbuntuManifestEditor::editorWidget() const
{
    return static_cast<UbuntuManifestEditorWidget*>(m_widget.data());
}

Core::IDocument *UbuntuManifestEditor::document()
{
    return editorWidget()->textEditorWidget()->baseTextDocument();
}

TextEditor::BaseTextEditorWidget *UbuntuManifestEditor::textEditor() const
{
    return editorWidget()->textEditorWidget();
}

int UbuntuManifestEditor::currentLine() const
{
    return textEditor()->textCursor().blockNumber() + 1;
}

int UbuntuManifestEditor::currentColumn() const
{
    QTextCursor cursor = textEditor()->textCursor();
    return cursor.position() - cursor.block().position() + 1;
}

void UbuntuManifestEditor::changeEditorPage(QAction *action)
{
    editorWidget()->setActivePage(static_cast<UbuntuManifestEditorWidget::EditorPage>(action->data().toInt()));
}

} // namespace Internal
} // namespace Ubuntu
