#include "ubuntuabstractguieditor.h"

#include "ubuntuabstractguieditorwidget.h"
#include "ubuntuconstants.h"

#include <texteditor/texteditorconstants.h>

#include <QActionGroup>
#include <QToolBar>
#include <QTextBlock>

namespace Ubuntu {
namespace Internal {

UbuntuAbstractGuiEditor::UbuntuAbstractGuiEditor(const Core::Id &id, const Core::Context &context)
    : Core::IEditor(), m_toolBar(0), m_actionGroup(0)
{
    setId(id);
    setContext(context);
}

bool UbuntuAbstractGuiEditor::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    if(editorWidget()->open(errorString,fileName,realFileName)){
        syncCurrentAction();
        return true;
    }
    return false;
}

QWidget *UbuntuAbstractGuiEditor::toolBar()
{
    return m_toolBar;
}

UbuntuAbstractGuiEditorWidget *UbuntuAbstractGuiEditor::editorWidget() const
{
    return static_cast<UbuntuAbstractGuiEditorWidget*>(m_widget.data());
}

Core::IDocument *UbuntuAbstractGuiEditor::document()
{
    return editorWidget()->textEditorWidget()->baseTextDocument();
}

TextEditor::BaseTextEditorWidget *UbuntuAbstractGuiEditor::textEditor() const
{
    return editorWidget()->textEditorWidget();
}

int UbuntuAbstractGuiEditor::currentLine() const
{
    return textEditor()->textCursor().blockNumber() + 1;
}

int UbuntuAbstractGuiEditor::currentColumn() const
{
    QTextCursor cursor = textEditor()->textCursor();
    return cursor.position() - cursor.block().position() + 1;
}

void UbuntuAbstractGuiEditor::syncCurrentAction()
{
    foreach (QAction *action, m_actionGroup->actions()) {
        if (action->data().toInt() == editorWidget()->activePage()) {
            action->setChecked(true);
            break;
        }
    }
}

void UbuntuAbstractGuiEditor::createUi()
{
    m_widget = createGuiEditor();

    m_toolBar = new QToolBar(m_widget.data());
    m_actionGroup = new QActionGroup(m_widget.data());
    connect(m_actionGroup, SIGNAL(triggered(QAction*)), this, SLOT(changeEditorPage(QAction*)));

    QAction *generalAction = m_toolBar->addAction(tr("General"));
    generalAction->setData(UbuntuAbstractGuiEditorWidget::General);
    generalAction->setCheckable(true);
    m_actionGroup->addAction(generalAction);

    QAction *sourceAction = m_toolBar->addAction(tr("JSON Source"));
    sourceAction->setData(UbuntuAbstractGuiEditorWidget::Source);
    sourceAction->setCheckable(true);
    m_actionGroup->addAction(sourceAction);

    generalAction->setChecked(true);

    setWidget(editorWidget());
}

void UbuntuAbstractGuiEditor::changeEditorPage(QAction *action)
{
    if(!editorWidget()->setActivePage(static_cast<UbuntuAbstractGuiEditorWidget::EditorPage>(action->data().toInt()))){
        syncCurrentAction();
    }
}

} // namespace Internal
} // namespace Ubuntu
