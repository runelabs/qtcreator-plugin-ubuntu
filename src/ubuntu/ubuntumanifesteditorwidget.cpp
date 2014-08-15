#include "ubuntumanifesteditorwidget.h"
#include "ubuntuconstants.h"
#include "ubuntumanifestdocument.h"
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
        ProjectExplorer::Kit *kit = project->activeTarget()->kit();
        ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(kit);
        if (tc && tc->type() == QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID)
                && fileName.isChildOf(Utils::FileName::fromString(project->projectDirectory())))
            return project;
    }
    return 0;
}

UbuntuManifestEditorWidget::UbuntuManifestEditorWidget(UbuntuManifestEditor *editor)
    : QScrollArea(),
      m_ui(0),
      m_editor(editor),
      m_widgetStack(0),
      m_sourceEditor(0),
      m_dirty(false)
{
    //make sure this is cleaned up when the editor is deleted
    connect(editor,SIGNAL(destroyed()),this,SLOT(deleteLater()));

    //this contains the source and GUI based editor
    m_widgetStack = new QStackedWidget;
    m_ui = new Ui::UbuntuManifestEditor();

    QWidget *mainWidget = new QWidget();
    m_ui->setupUi(mainWidget);

    m_sourceEditor = new UbuntuManifestTextEditorWidget(this);
    m_widgetStack->insertWidget(General,mainWidget);
    m_widgetStack->insertWidget(Source,m_sourceEditor);
    setWidgetResizable(true);
    setWidget(m_widgetStack);

    connect(m_ui->comboBoxFramework,SIGNAL(currentIndexChanged(int)),this,SLOT(setDirty()));
    connect(m_ui->lineEdit_description,SIGNAL(textChanged(QString)),this,SLOT(setDirty()));
    connect(m_ui->lineEdit_maintainer,SIGNAL(textChanged(QString)),this,SLOT(setDirty()));
    connect(m_ui->lineEdit_name,SIGNAL(textChanged(QString)),this,SLOT(setDirty()));
    connect(m_ui->lineEdit_title,SIGNAL(textChanged(QString)),this,SLOT(setDirty()));
    connect(m_ui->lineEdit_version,SIGNAL(textChanged(QString)),this,SLOT(setDirty()));

}

UbuntuManifestEditorWidget::~UbuntuManifestEditorWidget()
{
    delete m_ui;
}

bool UbuntuManifestEditorWidget::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    bool result = m_sourceEditor->open(errorString, fileName, realFileName);

    if(!result)
        return result;

    //let see if we have valid data
    m_manifest = QSharedPointer<UbuntuClickManifest>(new UbuntuClickManifest);
    if(m_manifest->loadFromString(m_sourceEditor->toPlainText())) {
        if(activePage() != Source)
            syncToWidgets(m_manifest.data());
        return true;
    } else {
        //switch to source page without syncing
        m_widgetStack->setCurrentIndex(Source);
        updateInfoBar(tr("There is a error in the file, please check the syntax."));
    }

    //ops something went wrong, we need to show the error somewhere
    return true;
}

bool UbuntuManifestEditorWidget::isModified() const
{
    return m_dirty;
}

UbuntuManifestEditorWidget::EditorPage UbuntuManifestEditorWidget::activePage() const
{
    return static_cast<EditorPage>(m_widgetStack->currentIndex());
}

bool UbuntuManifestEditorWidget::setActivePage(UbuntuManifestEditorWidget::EditorPage page)
{
    if(page == static_cast<UbuntuManifestEditorWidget::EditorPage>(m_widgetStack->currentIndex()))
        return true;

    if(page == UbuntuManifestEditorWidget::Source)
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
 * \brief UbuntuManifestEditorWidget::preSave
 * Called right before the document is stored
 */
bool UbuntuManifestEditorWidget::preSave()
{
    //sync the current widget state to source, so we save the correct data
    if(activePage() == General)
        syncToSource();
    else
        return syncToWidgets();
    return true;
}

Core::IEditor *UbuntuManifestEditorWidget::editor() const
{
    return m_editor;
}

TextEditor::PlainTextEditorWidget *UbuntuManifestEditorWidget::textEditorWidget() const
{
    return m_sourceEditor;
}

void UbuntuManifestEditorWidget::setDirty()
{
    m_dirty = true;
    emit uiEditorChanged();
}

bool UbuntuManifestEditorWidget::syncToWidgets()
{
    QSharedPointer<UbuntuClickManifest> man(new UbuntuClickManifest);
    if(man->loadFromString(m_sourceEditor->toPlainText())) {
        m_manifest.swap(man);
        syncToWidgets(m_manifest.data());
        updateInfoBar(QString());
        return true;
    }

    QString text = tr("There is a error in the file, please check the syntax.");
    updateInfoBar(text);
    return false;
}

bool UbuntuManifestEditorWidget::syncToWidgets(UbuntuClickManifest *source)
{
    QString data = source->maintainer();
    if(data != m_ui->lineEdit_maintainer->text())
        m_ui->lineEdit_maintainer->setText(data);

    data = source->name();
    if(data != m_ui->lineEdit_name->text())
        m_ui->lineEdit_name->setText(data);

    data = source->title();
    if(data != m_ui->lineEdit_title->text())
        m_ui->lineEdit_title->setText(data);

    data = source->version();
    if(data != m_ui->lineEdit_version->text())
        m_ui->lineEdit_version->setText(data);

    data = source->description();
    if(data != m_ui->lineEdit_description->text())
        m_ui->lineEdit_description->setText(data);

    updateFrameworkList();
    int idx = m_ui->comboBoxFramework->findText(m_manifest->frameworkName());

    //disable the currentIndexChanged signal, we need to check outselves if
    //the data has changed
    m_ui->comboBoxFramework->blockSignals(true);
    QVariant fwData = m_ui->comboBoxFramework->currentData();

    //if the framework name is not valid set to empty item
    //just some data to easily find the unknown framework item without
    //using string compare
    if(idx < 0) {
        if(m_ui->comboBoxFramework->findData(Constants::UBUNTU_UNKNOWN_FRAMEWORK_DATA) < 0)
            m_ui->comboBoxFramework->addItem(tr(Constants::UBUNTU_UNKNOWN_FRAMEWORK_NAME),Constants::UBUNTU_UNKNOWN_FRAMEWORK_DATA);

        m_ui->comboBoxFramework->setCurrentIndex(m_ui->comboBoxFramework->count()-1);
    } else {
        m_ui->comboBoxFramework->setCurrentIndex(idx);
        m_ui->comboBoxFramework->removeItem(m_ui->comboBoxFramework->findData(Constants::UBUNTU_UNKNOWN_FRAMEWORK_DATA));
    }

    m_ui->comboBoxFramework->blockSignals(false);

    //set the dirty flag manually in case something has changed
    if(m_ui->comboBoxFramework->currentData() != fwData)
        setDirty();

    QSet<int> idxToKeep;
    QList<UbuntuClickManifest::Hook> hooks = source->hooks();
    foreach(const UbuntuClickManifest::Hook &hook, hooks) {
        int idx = m_ui->comboBoxHook->findText(hook.appId);
        QWidget *container = 0;

        //create a new one
        if(idx < 0) {
            container = createHookWidget(hook);
            m_ui->comboBoxHook->addItem(hook.appId);
            idx = m_ui->stackedWidget->addWidget(container);
        } else
            container = m_ui->stackedWidget->widget(idx);

        if(!hook.desktopFile.isEmpty()) {
            QLineEdit *desktop  = container->findChild<QLineEdit*>(hook.appId+QStringLiteral(".desktop"));
            QLineEdit *appArmor = container->findChild<QLineEdit*>(hook.appId+QStringLiteral(".apparmor"));
            if(desktop && desktop->text() != hook.desktopFile)
                desktop->setText(hook.desktopFile);
            if(appArmor && appArmor->text() != hook.appArmorFile)
                appArmor->setText(hook.appArmorFile);
        } else if (!hook.scope.isEmpty()){
            QLineEdit *scope  = container->findChild<QLineEdit*>(hook.appId+QStringLiteral(".scope"));
            QLineEdit *appArmor = container->findChild<QLineEdit*>(hook.appId+QStringLiteral(".apparmor"));
            if(scope && scope->text() != hook.scope)
                scope->setText(hook.scope);
            if(appArmor && appArmor->text() != hook.appArmorFile)
                appArmor->setText(hook.appArmorFile);
        }
        idxToKeep.insert(idx);
    }

    //clean up old widgets
    if(idxToKeep.size() != m_ui->comboBoxHook->count()) {
        for(int i = m_ui->comboBoxHook->count(); i >= 0; i--) {
            if(!idxToKeep.contains(i)) {
                m_ui->comboBoxHook->removeItem(i);
                QWidget* toRemove = m_ui->stackedWidget->widget(i);
                m_ui->stackedWidget->removeWidget(toRemove);
                delete toRemove;
            }
        }
    }

    m_dirty = false;
    emit uiEditorChanged();

    return true;
}

void UbuntuManifestEditorWidget::syncToSource()
{
    // set package name to lower, bug #1219877
    QString packageName = m_ui->lineEdit_name->text();
    static const QRegularExpression varCheck(QStringLiteral("@.*@"));
    if(!varCheck.match(packageName).hasMatch()) {
        packageName = packageName.toLower();
        m_ui->lineEdit_name->setText(packageName);
    }

    m_manifest->setName(packageName);
    m_manifest->setMaintainer(m_ui->lineEdit_maintainer->text());
    m_manifest->setVersion(m_ui->lineEdit_version->text());
    m_manifest->setTitle(m_ui->lineEdit_title->text());
    m_manifest->setDescription(m_ui->lineEdit_description->text());

    if(m_ui->comboBoxFramework->currentText() != tr(Constants::UBUNTU_UNKNOWN_FRAMEWORK_NAME))
        m_manifest->setFrameworkName(m_ui->comboBoxFramework->currentText());

    for(int idx = 0; idx < m_ui->comboBoxHook->count(); idx++) {
        QWidget *container  = m_ui->stackedWidget->widget(idx);
        QString appId       = m_ui->comboBoxHook->itemText(idx);
        QLineEdit *desktop  = container->findChild<QLineEdit*>(appId+QStringLiteral(".desktop"));
        QLineEdit *scope    = container->findChild<QLineEdit*>(appId+QStringLiteral(".scope"));
        QLineEdit *appArmor = container->findChild<QLineEdit*>(appId+QStringLiteral(".apparmor"));

        UbuntuClickManifest::Hook hook;
        hook.appId = appId;
        hook.appArmorFile = appArmor->text();

        if(desktop)
            hook.desktopFile = desktop->text();
        else if(scope)
            hook.scope = desktop->text();
        else
            //What to do here, this should never happen
            continue;

        m_manifest->setHook(hook);
    }


    QString result = m_manifest->raw()+QStringLiteral("\n");
    QString src    = m_sourceEditor->toPlainText();
    if (result == src)
        return;

    m_sourceEditor->setPlainText(result);
    m_sourceEditor->document()->setModified(true);

    m_dirty = false;
    emit uiEditorChanged();
}

void UbuntuManifestEditorWidget::updateFrameworkList()
{
    const QString docPath(m_sourceEditor->baseTextDocument()->filePath());

    const UbuntuClickTool::Target *t = 0;
    ProjectExplorer::Project* myProject = ubuntuProject(docPath);
    if (myProject)
        t = UbuntuClickTool::clickTargetFromTarget(myProject->activeTarget());

    m_ui->comboBoxFramework->blockSignals(true);
    m_ui->comboBoxFramework->clear();
    m_ui->comboBoxFramework->addItems(UbuntuClickTool::getSupportedFrameworks(t));
    m_ui->comboBoxFramework->blockSignals(false);
}

void UbuntuManifestEditorWidget::updateInfoBar(const QString &errorMessage)
{
    Core::InfoBar *infoBar = m_sourceEditor->baseTextDocument()->infoBar();
    infoBar->removeInfo(infoBarId);

    if(!errorMessage.isEmpty()){
        Core::InfoBarEntry infoBarEntry(infoBarId, errorMessage);
        infoBar->addInfo(infoBarEntry);
    }
}


QWidget *UbuntuManifestEditorWidget::createHookWidget(const UbuntuClickManifest::Hook &hook)
{
    QWidget *container = new QWidget(m_ui->stackedWidget);
    QVBoxLayout *layout = new QVBoxLayout(container);
    if(!hook.desktopFile.isEmpty()) {
        //App hook
        QLabel* label = new QLabel(tr("Desktop file"));
        layout->addWidget(label);

        QLineEdit *lE = new QLineEdit();
        lE->setObjectName(hook.appId+QStringLiteral(".desktop"));
        connect(lE,SIGNAL(textChanged(QString)),this,SLOT(setDirty()));
        layout->addWidget(lE);

        label = new QLabel(tr("Apparmor file"));
        layout->addWidget(label);

        lE = new QLineEdit();
        lE->setObjectName(hook.appId+QStringLiteral(".apparmor"));
        connect(lE,SIGNAL(textChanged(QString)),this,SLOT(setDirty()));
        layout->addWidget(lE);
    } else if(!hook.scope.isEmpty()) {
        //Scope hook
        QLabel* label = new QLabel(tr("Scope ini file"));
        layout->addWidget(label);

        QLineEdit *lE = new QLineEdit();
        lE->setObjectName(hook.appId+QStringLiteral(".scope"));
        connect(lE,SIGNAL(textChanged(QString)),this,SLOT(setDirty()));
        layout->addWidget(lE);

        label = new QLabel(tr("Apparmor file"));
        layout->addWidget(label);

        lE = new QLineEdit();
        lE->setObjectName(hook.appId+QStringLiteral(".apparmor"));
        connect(lE,SIGNAL(textChanged(QString)),this,SLOT(setDirty()));
        layout->addWidget(lE);
    }
    layout->addStretch();
    container->setLayout(layout);
    return container;
}

UbuntuManifestTextEditorWidget::UbuntuManifestTextEditorWidget(UbuntuManifestEditorWidget *parent)
    : TextEditor::PlainTextEditorWidget(new UbuntuManifestDocument(parent), parent),
      m_parent(parent)
{
    baseTextDocument()->setMimeType(QLatin1String(Constants::UBUNTU_MANIFEST_MIME_TYPE));
}



} // namespace Internal
} // namespace Ubuntu
