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

#include "ubuntumanifesteditorwidget.h"
#include "ubuntuconstants.h"
#include "ubuntuabstractguieditordocument.h"
#include "ubuntumanifesteditor.h"
#include "ubuntuapparmoreditor.h"
#include "ubuntuclicktool.h"
#include "clicktoolchain.h"
#include "ubuntubzr.h"

#include <projectexplorer/session.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/target.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <coreplugin/infobar.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/texteditor.h>

#include <QStackedWidget>
#include <QDebug>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace Ubuntu {
namespace Internal {

enum {
    debug = 0
};

UbuntuManifestEditorWidget::UbuntuManifestEditorWidget()
    : UbuntuAbstractGuiEditorWidget(QLatin1String(Constants::UBUNTU_MANIFEST_MIME_TYPE)),
      m_ui(0)
{
    createUI();

    UbuntuBzr *bzr = UbuntuBzr::instance();
    connect(bzr,SIGNAL(initializedChanged()),SLOT(bzrChanged()));
    if(bzr->isInitialized())
        bzrChanged();
}

QWidget *UbuntuManifestEditorWidget::createMainWidget()
{
    Q_ASSERT_X(m_ui == 0,Q_FUNC_INFO,"createMainWidget was called multiple times");

    QWidget *w = new QWidget();
    m_ui = new Ui::UbuntuManifestEditor();
    m_ui->setupUi(w);

    connect(m_ui->comboBoxFramework,SIGNAL(currentIndexChanged(int)),this,SLOT(onFrameworkChanged()));
    connect(m_ui->lineEdit_description,SIGNAL(textChanged(QString)),this,SLOT(setDirty()));
    connect(m_ui->lineEdit_maintainer,SIGNAL(textChanged(QString)),this,SLOT(setDirty()));
    connect(m_ui->lineEdit_name,SIGNAL(textChanged(QString)),this,SLOT(setDirty()));
    connect(m_ui->lineEdit_title,SIGNAL(textChanged(QString)),this,SLOT(setDirty()));
    connect(m_ui->lineEdit_version,SIGNAL(textChanged(QString)),this,SLOT(setDirty()));
    connect(UbuntuClickFrameworkProvider::instance(),SIGNAL(frameworksUpdated()),this,SLOT(updateFrameworkList()));

    updateFrameworkList();

    return w;
}


UbuntuManifestEditorWidget::~UbuntuManifestEditorWidget()
{
    delete m_ui;
}

bool UbuntuManifestEditorWidget::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    addMissingFieldsToManifest(realFileName);

    bool result = UbuntuAbstractGuiEditorWidget::open(errorString,fileName,realFileName);

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

    //disable the currentIndexChanged signal, we need to check outselves if
    //the data has changed
    m_ui->comboBoxFramework->blockSignals(true);

    QString fwText = m_ui->comboBoxFramework->currentText();
    selectFramework(source->frameworkName());

    m_ui->comboBoxFramework->blockSignals(false);

    //set the dirty flag manually in case something has changed
    if(m_ui->comboBoxFramework->currentText() != fwText)
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

    if(m_ui->comboBoxFramework->currentData() != Constants::UBUNTU_UNKNOWN_FRAMEWORK_DATA)
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
            hook.scope = scope->text();
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
    m_ui->comboBoxFramework->blockSignals(true);

    //the current selected fw
    QString fwText = m_ui->comboBoxFramework->currentText();

    m_ui->comboBoxFramework->clear();
    m_ui->comboBoxFramework->addItems(UbuntuClickFrameworkProvider::getSupportedFrameworks());
    selectFramework(fwText);
    m_ui->comboBoxFramework->blockSignals(false);
}

void UbuntuManifestEditorWidget::selectFramework (const QString &fw)
{
    // get the new Index for our new fw
    int idx = m_ui->comboBoxFramework->findText(fw);

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
}

void UbuntuManifestEditorWidget::bzrChanged()
{
    UbuntuBzr *bzr = UbuntuBzr::instance();

    m_ui->lineEdit_maintainer->setText(bzr->whoami());

    /* Commented out for bug #1219948  - https://bugs.launchpad.net/qtcreator-plugin-ubuntu/+bug/1219948
    QString userName = bzr->launchpadId();
    if (userName.isEmpty()) userName = QLatin1String(Constants::USERNAME);
    m_ui->lineEdit_maintainer->setText(QString(QLatin1String("%0.%1")).arg(m_projectName).arg(userName));
    */

    if(activePage() != General)
        syncToSource();
}

void UbuntuManifestEditorWidget::onFrameworkChanged()
{
    //make sure all changes are in the manifest instance
    syncToSource();

    if(m_ui->comboBoxFramework->currentData() != Constants::UBUNTU_UNKNOWN_FRAMEWORK_DATA) {
        int idx = m_ui->comboBoxFramework->findData(Constants::UBUNTU_UNKNOWN_FRAMEWORK_DATA);
        if(idx >= 0)
            m_ui->comboBoxFramework->removeItem(idx);
    }

    QString v = UbuntuClickFrameworkProvider::instance()->frameworkPolicy(m_ui->comboBoxFramework->currentText());
    if(v.isEmpty())
        return;

    QList<UbuntuClickManifest::Hook> hooks = m_manifest->hooks();
    foreach(const UbuntuClickManifest::Hook &hook, hooks){
        QFileInfo mFile = textEditorWidget()->textDocument()->filePath().toFileInfo();
        QString aaFile = mFile.absolutePath()+QDir::separator()+hook.appArmorFile;
        if(!QFile::exists(aaFile)) {

            ProjectExplorer::Project *p = ubuntuProject(mFile.absolutePath());
            if(!p)
                continue;

            //the aa file does not live in the same directory as the manifest file
            //try if we can use the project root directory to find the file
            aaFile = p->projectDirectory().appendPath(hook.appArmorFile).toString();
            if(!QFile::exists(aaFile))
                continue;
        }

        QList<Core::IEditor*> editors = Core::DocumentModel::editorsForFilePath(aaFile);
        if(editors.size()) {
            if(debug) qDebug()<<"Write into editor contents";

            foreach(Core::IEditor *ed, editors){
                //first check if we have a ubuntu apparmor editor
                UbuntuApparmorEditor *uEd = qobject_cast<UbuntuApparmorEditor *>(ed);
                if(uEd) {
                    uEd->guiEditor()->setVersion(v);
                    break;
                }

                //ok that is no ubuntu editor, lets check if its a basic texteditor and just
                //replace the contents
                TextEditor::BaseTextEditor *txtEd = qobject_cast<TextEditor::BaseTextEditor *>(ed);
                if(txtEd) {
                    //ok this is more complicated, first lets get the contents
                    UbuntuClickManifest aa;
                    if(aa.loadFromString(txtEd->textDocument()->plainText())){
                        aa.setPolicyVersion(v);
                        txtEd->textDocument()->setPlainText(aa.raw());
                        txtEd->textDocument()->document()->setModified(true);
                        break;
                    }
                }
            }
        } else {
            if(debug) qDebug()<<"Write the closed file";
            UbuntuClickManifest aa;
            if(aa.load(aaFile)) {
                aa.setPolicyVersion(v);
                aa.save();
            }
        }
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

QString UbuntuManifestEditorWidget::createPackageName(const QString &userName, const QString &projectName)
{
    return QString(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_DEFAULT_NAME)).arg(projectName).arg(userName);
}

void UbuntuManifestEditorWidget::addMissingFieldsToManifest (QString fileName)
{
    QFile in(fileName);
    if(!in.open(QIODevice::ReadOnly))
        return;

    QFile templateFile(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_DEFAULT_MANIFEST));
    if(!templateFile.open(QIODevice::ReadOnly))
        return;

    QString projectName = tr("unknown");
    ProjectExplorer::Project *p = ubuntuProject(fileName);
    if(p)
        projectName = p->displayName();

    QJsonParseError err;
    QJsonDocument templateDoc = QJsonDocument::fromJson(templateFile.readAll(),&err);
    if(err.error != QJsonParseError::NoError)
        return;

    QJsonDocument inDoc = QJsonDocument::fromJson(in.readAll(),&err);
    if(err.error != QJsonParseError::NoError)
        return;

    in.close();

    QJsonObject templateObject = templateDoc.object();
    QJsonObject targetObject = inDoc.object();
    QJsonObject::const_iterator i = templateObject.constBegin();

    UbuntuBzr *bzr = UbuntuBzr::instance();

    bool changed = false;
    for(;i != templateObject.constEnd(); i++) {
        if(!targetObject.contains(i.key())) {
            changed = true;

            if(debug) qDebug()<<"Manifest file missing key: "<<i.key();

            if (i.key() == QStringLiteral("name")) {
                QString userName = bzr->launchpadId();
                if (userName.isEmpty()) userName = QLatin1String(Constants::USERNAME);
                targetObject.insert(i.key(),createPackageName(userName,projectName));

                if(debug) qDebug()<<"Setting to "<<createPackageName(userName,projectName);
            } else if (i.key() == QStringLiteral("maintainer")) {
                targetObject.insert(i.key(),bzr->whoami());

                if(debug) qDebug()<<"Setting to "<<bzr->whoami();
            } else if (i.key() == QStringLiteral("framework")) {
                targetObject.insert(i.key(),UbuntuClickFrameworkProvider::getMostRecentFramework( QString()));
            } else {
                targetObject.insert(i.key(),i.value());
                if(debug) qDebug() <<"Setting to "<<i.value();
            }
        }
    }

    if (changed) {
        if(!in.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return;
        QJsonDocument doc(targetObject);
        QByteArray data = doc.toJson();
        in.write(data);
        in.close();
    }
}

/*!
 * \brief UbuntuManifestEditorWidget::saved
 * Runs updateDefaultRunConfigurations for the currently
 * active target, to make sure all changes in the manifest
 * file are taken into account
 */
void UbuntuManifestEditorWidget::saved()
{
    QFileInfo mFile = textEditorWidget()->textDocument()->filePath().toFileInfo();
    ProjectExplorer::Project *p = ubuntuProject(mFile.absoluteFilePath());
    if(p && p->activeTarget())
        p->activeTarget()->updateDefaultRunConfigurations();
}

} // namespace Internal
} // namespace Ubuntu
