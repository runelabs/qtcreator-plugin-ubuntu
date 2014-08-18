#ifndef UBUNTU_INTERNAL_UBUNTUMANIFESTDOCUMENT_H
#define UBUNTU_INTERNAL_UBUNTUMANIFESTDOCUMENT_H

#include <coreplugin/textdocument.h>
#include <texteditor/plaintexteditor.h>

namespace Ubuntu {
namespace Internal {

class UbuntuAbstractGuiEditorWidget;

class UbuntuAbstractGuiEditorDocument : public TextEditor::PlainTextDocument
{
public:
    UbuntuAbstractGuiEditorDocument(const QString &mimeType, UbuntuAbstractGuiEditorWidget *editorWidget);
    bool save(QString *errorString, const QString &fileName = QString(), bool autoSave = false);

    QString defaultPath() const override;
    QString suggestedFileName() const override;

    bool isModified() const override;
    bool isSaveAsAllowed() const override;
private:
    UbuntuAbstractGuiEditorWidget *m_editorWidget;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUMANIFESTDOCUMENT_H
