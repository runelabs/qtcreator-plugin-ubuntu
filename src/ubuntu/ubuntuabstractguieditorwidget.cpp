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

#include <QStackedWidget>
#include <QDebug>


namespace Ubuntu {
namespace Internal {

const char infoBarId[] = "UbuntuProjectManager.UbuntuManifestEditor.InfoBar";

//try to find the project by the file path
ProjectExplorer::Project *ubuntuProject(const QString &file)
{
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
        if(fileName.isChildOf(Utils::FileName::fromString(project->projectDirectory())))
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
}

UbuntuAbstractGuiEditorWidget::~UbuntuAbstractGuiEditorWidget()
{
}

bool UbuntuAbstractGuiEditorWidget::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    bool result = m_sourceEditor->open(errorString, fileName, realFileName);

    if(!result)
        return result;
    return true;
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

TextEditor::PlainTextEditorWidget *UbuntuAbstractGuiEditorWidget::textEditorWidget() const
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


void UbuntuAbstractGuiEditorWidget::updateInfoBar(const QString &errorMessage)
{
    Core::InfoBar *infoBar = m_sourceEditor->baseTextDocument()->infoBar();
    infoBar->removeInfo(infoBarId);

    if(!errorMessage.isEmpty()){
        Core::InfoBarEntry infoBarEntry(infoBarId, errorMessage);
        infoBar->addInfo(infoBarEntry);
    }
}

UbuntuManifestTextEditorWidget::UbuntuManifestTextEditorWidget(QString mimeType, UbuntuAbstractGuiEditorWidget *parent)
    : TextEditor::PlainTextEditorWidget(new UbuntuAbstractGuiEditorDocument(mimeType,parent), parent),
      m_parent(parent)
{
    baseTextDocument()->setMimeType(mimeType);
}



} // namespace Internal
} // namespace Ubuntu
