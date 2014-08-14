#include "ubuntumanifestdocument.h"
#include "ubuntumanifesteditor.h"
#include "ubuntumanifesteditorwidget.h"
#include "ubuntuconstants.h"

#include <coreplugin/editormanager/ieditor.h>

#include <QDir>

namespace Ubuntu {
namespace Internal {

/*!
 * \class UbuntuManifestDocument::UbuntuManifestDocument
 *  This is basically a bare PlainTextDocument, however in order to be
 *  able to sync between the Widget based and Text based manifest editor
 *  we need to hook into save() and isModified()
 */


UbuntuManifestDocument::UbuntuManifestDocument(UbuntuManifestEditorWidget *editorWidget)
    : TextEditor::PlainTextDocument(),
      m_editorWidget(editorWidget)
{
    setMimeType(QLatin1String(Constants::UBUNTU_MANIFEST_MIME_TYPE));
    connect(editorWidget, SIGNAL(uiEditorChanged()),this, SIGNAL(changed()));
}

bool UbuntuManifestDocument::save(QString *errorString, const QString &fileName, bool autoSave)
{
    m_editorWidget->preSave();
    return BaseTextDocument::save(errorString, fileName, autoSave);
}

QString UbuntuManifestDocument::defaultPath() const
{
    QFileInfo fi(filePath());
    return fi.absolutePath();
}

QString UbuntuManifestDocument::suggestedFileName() const
{
    QFileInfo fi(filePath());
    return fi.fileName();
}

bool UbuntuManifestDocument::isModified() const
{
    return BaseTextDocument::isModified() ||  m_editorWidget->isModified();
}

bool UbuntuManifestDocument::isSaveAsAllowed() const
{
    return false;
}

} // namespace Internal
} // namespace Ubuntu
