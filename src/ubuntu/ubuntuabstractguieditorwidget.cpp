/*
 * Copyright 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "ubuntuabstractguieditorwidget.h"

#include "ubuntumanifesteditorwidget.h"
#include "ubuntuconstants.h"
#include "ubuntuabstractguieditordocument.h"
#include "ubuntumanifesteditor.h"
#include "ubuntuclicktool.h"
#include "clicktoolchain.h"

#include <projectexplorer/session.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/target.h>
#include <projectexplorer/kitinformation.h>
#include <coreplugin/infobar.h>
#include <coreplugin/idocument.h>

#include <QStackedWidget>
#include <QDebug>


namespace Ubuntu {
namespace Internal {

const char infoBarId[] = "UbuntuProjectManager.UbuntuManifestEditor.InfoBar";

//try to find the project by the file path
ProjectExplorer::Project *ubuntuProject(const QString &file)
{
    //Project *SessionManager::projectForFile(const Utils::FileName &fileName)
    Utils::FileName fileName = Utils::FileName::fromString(file);
    foreach (ProjectExplorer::Project *project, ProjectExplorer::SessionManager::projects()) {
        if (!project->activeTarget())
            continue;
#if 0
        ProjectExplorer::Kit *kit = project->activeTarget()->kit();
        ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(kit);
        if (tc && tc->type() == QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID)
                && fileName.isChildOf(Utils::FileName::fromString(project->projectDirectory())))
#endif
        if(fileName.isChildOf(project->projectDirectory()))
            return project;
    }
    return 0;
}

UbuntuAbstractGuiEditorWidget::UbuntuAbstractGuiEditorWidget(const QString &mimeType)
    : QScrollArea(),
      m_widgetStack(0),
      m_sourceEditor(0),
      m_dirty(false)
{
    //this contains the source and GUI based editor
    m_widgetStack = new QStackedWidget;
    m_sourceEditor = new UbuntuManifestTextEditorWidget(mimeType, this);
    setWidgetResizable(true);
    setWidget(m_widgetStack);

    connect(m_sourceEditor->textDocument(), &TextEditor::TextDocument::aboutToOpen,
            this, &UbuntuAbstractGuiEditorWidget::aboutToOpen);
    connect(m_sourceEditor->textDocument(), &TextEditor::TextDocument::reloadFinished,
            this, [this](bool success) { if (success) updateAfterFileLoad(); });
    connect(m_sourceEditor->textDocument(), &TextEditor::TextDocument::openFinishedSuccessfully,
            this, &UbuntuAbstractGuiEditorWidget::updateAfterFileLoad);
    connect(m_widgetStack, &QStackedWidget::currentChanged,
            this, &UbuntuAbstractGuiEditorWidget::editorViewChanged);
}

UbuntuAbstractGuiEditorWidget::~UbuntuAbstractGuiEditorWidget()
{
}

void UbuntuAbstractGuiEditorWidget::aboutToOpen(const QString &, const QString &)
{
    //do nothing
}

void UbuntuAbstractGuiEditorWidget::updateAfterFileLoad()
{
    //do nothing
}

bool UbuntuAbstractGuiEditorWidget::isModified() const
{
    return m_dirty;
}

UbuntuAbstractGuiEditorWidget::EditorPage UbuntuAbstractGuiEditorWidget::activePage() const
{
    return static_cast<EditorPage>(m_widgetStack->currentIndex());
}

bool UbuntuAbstractGuiEditorWidget::setActivePage(UbuntuAbstractGuiEditorWidget::EditorPage page)
{
    if(page == static_cast<UbuntuAbstractGuiEditorWidget::EditorPage>(m_widgetStack->currentIndex()))
        return true;

    if(page == UbuntuAbstractGuiEditorWidget::Source)
        syncToSource();
    else {
        //this could fail because the user edited the source manually
        if(!syncToWidgets()){
            return false;
        }
    }

    m_widgetStack->setCurrentIndex(page);
    return true;
}

/*!
 * \brief UbuntuAbstractGuiEditorWidget::preSave
 * Called right before the document is stored
 */
bool UbuntuAbstractGuiEditorWidget::preSave()
{
    //sync the current widget state to source, so we save the correct data
    if(activePage() == General)
        syncToSource();
    else
        return syncToWidgets();
    return true;
}

TextEditor::TextEditorWidget *UbuntuAbstractGuiEditorWidget::textEditorWidget() const
{
    return m_sourceEditor;
}

void UbuntuAbstractGuiEditorWidget::setDirty()
{
    m_dirty = true;
    emit uiEditorChanged();
}

void UbuntuAbstractGuiEditorWidget::createUI()
{
    m_widgetStack->insertWidget(General,createMainWidget());
    m_widgetStack->insertWidget(Source,m_sourceEditor);
}

void UbuntuAbstractGuiEditorWidget::saved()
{
    //default impl does nothing
}


void UbuntuAbstractGuiEditorWidget::updateInfoBar(const QString &errorMessage)
{
    Core::InfoBar *infoBar = m_sourceEditor->textDocument()->infoBar();
    infoBar->removeInfo(infoBarId);

    if(!errorMessage.isEmpty()){
        Core::InfoBarEntry infoBarEntry(infoBarId, errorMessage);
        infoBar->addInfo(infoBarEntry);
    }
}

UbuntuManifestTextEditorWidget::UbuntuManifestTextEditorWidget(QString mimeType, UbuntuAbstractGuiEditorWidget *parent)
    : TextEditor::TextEditorWidget(parent),
      m_parent(parent),
      m_mimeType(mimeType)
{
    setTextDocument(TextEditor::TextDocumentPtr(new UbuntuAbstractGuiEditorDocument(mimeType,parent)));
    textDocument()->setMimeType(mimeType);
    setupGenericHighlighter();
}



} // namespace Internal
} // namespace Ubuntu
