#include "ubuntumanifesteditorfactory.h"
#include "ubuntumanifesteditor.h"
#include "ubuntuconstants.h"

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
        UbuntuManifestEditor *ubuntuManifestEditor = static_cast<UbuntuManifestEditor *>(editor);
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

} // namespace Internal
} // namespace Ubuntu
