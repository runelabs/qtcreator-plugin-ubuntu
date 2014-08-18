#include "ubuntumanifesteditor.h"
#include "ubuntumanifesteditorwidget.h"
#include "ubuntuconstants.h"

#include <texteditor/texteditorconstants.h>

#include <QActionGroup>
#include <QToolBar>
#include <QTextBlock>

namespace Ubuntu {
namespace Internal {

UbuntuManifestEditor::UbuntuManifestEditor()
    : UbuntuAbstractGuiEditor(Constants::UBUNTU_MANIFEST_EDITOR_ID,
                              Core::Context(Constants::UBUNTU_MANIFEST_EDITOR_CONTEXT)),
      m_editorWidget(0)
{
    createUi();
}

UbuntuManifestEditor::~UbuntuManifestEditor()
{
    if(m_editorWidget)
        delete m_editorWidget;
}

UbuntuAbstractGuiEditorWidget *UbuntuManifestEditor::createGuiEditor()
{
    if(m_editorWidget == 0)
        m_editorWidget = new UbuntuManifestEditorWidget();

    return m_editorWidget;
}

} // namespace Internal
} // namespace Ubuntu
