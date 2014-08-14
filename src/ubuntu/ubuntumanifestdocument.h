#ifndef UBUNTU_INTERNAL_UBUNTUMANIFESTDOCUMENT_H
#define UBUNTU_INTERNAL_UBUNTUMANIFESTDOCUMENT_H

#include <coreplugin/textdocument.h>
#include <texteditor/plaintexteditor.h>

namespace Ubuntu {
namespace Internal {

class UbuntuManifestEditorWidget;

class UbuntuManifestDocument : public TextEditor::PlainTextDocument
{
public:
    UbuntuManifestDocument(UbuntuManifestEditorWidget *editorWidget);
    bool save(QString *errorString, const QString &fileName = QString(), bool autoSave = false);

    QString defaultPath() const override;
    QString suggestedFileName() const override;

    bool isModified() const override;
    bool isSaveAsAllowed() const override;
private:
    UbuntuManifestEditorWidget *m_editorWidget;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUMANIFESTDOCUMENT_H
