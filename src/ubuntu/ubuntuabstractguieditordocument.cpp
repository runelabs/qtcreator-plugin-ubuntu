#include "ubuntuabstractguieditordocument.h"
#include "ubuntuabstractguieditor.h"
#include "ubuntuabstractguieditorwidget.h"
#include "ubuntuconstants.h"

#include <coreplugin/editormanager/ieditor.h>

#include <QDir>

namespace Ubuntu {
namespace Internal {

/*!
 * \class UbuntuAbstractGuiEditorDocument::UbuntuAbstractGuiEditorDocument
 *  This is basically a bare PlainTextDocument, however in order to be
 *  able to sync between the Widget based and Text based manifest editor
 *  we need to hook into save() and isModified()
 */
UbuntuAbstractGuiEditorDocument::UbuntuAbstractGuiEditorDocument(const QString &mimeType, UbuntuAbstractGuiEditorWidget *editorWidget)
    : TextEditor::PlainTextDocument(),
      m_editorWidget(editorWidget)
{
    setMimeType(mimeType);
    connect(editorWidget, SIGNAL(uiEditorChanged()),this,SIGNAL(changed()));
}

bool UbuntuAbstractGuiEditorDocument::save(QString *errorString, const QString &fileName, bool autoSave)
{
    if(!m_editorWidget->preSave()) {
        *errorString = tr("Please check the info box in the editor.");
        return false;
    }

    return BaseTextDocument::save(errorString, fileName, autoSave);
}

QString UbuntuAbstractGuiEditorDocument::defaultPath() const
{
    QFileInfo fi(filePath());
    return fi.absolutePath();
}

QString UbuntuAbstractGuiEditorDocument::suggestedFileName() const
{
    QFileInfo fi(filePath());
    return fi.fileName();
}

bool UbuntuAbstractGuiEditorDocument::isModified() const
{
    return BaseTextDocument::isModified() ||  m_editorWidget->isModified();
}

bool UbuntuAbstractGuiEditorDocument::isSaveAsAllowed() const
{
    return false;
}

} // namespace Internal
} // namespace Ubuntu
