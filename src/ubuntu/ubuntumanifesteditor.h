/*
 * Copyright 2014 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.1.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Benjamin Zeller <benjamin.zeller@canonical.com>
 */

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
    virtual UbuntuAbstractGuiEditorWidget *createGuiEditor() override;

private:
    UbuntuManifestEditorWidget *m_editorWidget;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUMANIFESTEDITOR_H
