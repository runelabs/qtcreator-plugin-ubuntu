#ifndef UBUNTU_INTERNAL_UBUNTUMANIFESTEDITOR_H
#define UBUNTU_INTERNAL_UBUNTUMANIFESTEDITOR_H

#include "ubuntuabstractguieditor.h"

class QToolBar;

namespace Ubuntu {
namespace Internal {

class UbuntuManifestEditorWidget;

class UbuntuManifestEditor : public UbuntuAbstractGuiEditor
{
    Q_OBJECT
public:
    UbuntuManifestEditor();
    ~UbuntuManifestEditor();

    // UbuntuAbstractGuiEditor interface
protected:
    virtual UbuntuAbstractGuiEditorWidget *createGuiEditor();

private:
    UbuntuManifestEditorWidget *m_editorWidget;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUMANIFESTEDITOR_H
