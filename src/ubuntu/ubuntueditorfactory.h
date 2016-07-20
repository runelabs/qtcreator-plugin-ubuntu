#ifndef UBUNTU_INTERNAL_UBUNTUMANIFESTEDITORFACTORY_H
#define UBUNTU_INTERNAL_UBUNTUMANIFESTEDITORFACTORY_H

#include <coreplugin/editormanager/ieditorfactory.h>

namespace Ubuntu {
namespace Internal {

class UbuntuManifestEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT
public:
    explicit UbuntuManifestEditorFactory();
    Core::IEditor *createEditor() override;
};

class UbuntuApparmorEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT
public:
    explicit UbuntuApparmorEditorFactory();
    Core::IEditor *createEditor() override;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUMANIFESTEDITORFACTORY_H
