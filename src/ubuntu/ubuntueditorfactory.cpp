#include "ubuntueditorfactory.h"
#include "ubuntumanifesteditor.h"
#include "ubuntuconstants.h"
#include "ubuntuapparmoreditor.h"

#include <coreplugin/id.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorsettings.h>

namespace Ubuntu {
namespace Internal {

class UbuntuTextEditorActionHandler : public TextEditor::TextEditorActionHandler
{
public:
    explicit UbuntuTextEditorActionHandler(QObject *parent)
        : TextEditorActionHandler(parent, Constants::UBUNTU_MANIFEST_EDITOR_CONTEXT)
    {}
private:
    TextEditor::BaseTextEditorWidget *resolveTextEditorWidget(Core::IEditor *editor) const
    {
        UbuntuAbstractGuiEditor *ubuntuManifestEditor = static_cast<UbuntuAbstractGuiEditor *>(editor);
        return ubuntuManifestEditor->textEditor();
    }
};

UbuntuManifestEditorFactory::UbuntuManifestEditorFactory()
{
    setId(Constants::UBUNTU_MANIFEST_EDITOR_ID);
    setDisplayName(tr("Ubuntu Manifest editor"));
    addMimeType(Constants::UBUNTU_MANIFEST_MIME_TYPE);
    new UbuntuTextEditorActionHandler(this);
}

Core::IEditor *UbuntuManifestEditorFactory::createEditor()
{
    return new UbuntuManifestEditor();
}

UbuntuApparmorEditorFactory::UbuntuApparmorEditorFactory()
{
    setId(Constants::UBUNTU_APPARMOR_EDITOR_ID);
    setDisplayName(tr("Ubuntu Apparmor editor"));
    addMimeType(Constants::UBUNTU_APPARMOR_MIME_TYPE);
    new UbuntuTextEditorActionHandler(this);
}

Core::IEditor *UbuntuApparmorEditorFactory::createEditor()
{
    return new UbuntuApparmorEditor();
}

} // namespace Internal
} // namespace Ubuntu
