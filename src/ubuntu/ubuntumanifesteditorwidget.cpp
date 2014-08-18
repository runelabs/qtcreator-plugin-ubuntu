#include "ubuntumanifesteditorwidget.h"
#include "ubuntuconstants.h"
#include "ubuntuabstractguieditordocument.h"
#include "ubuntumanifesteditor.h"
#include "ubuntuclicktool.h"
#include "clicktoolchain.h"
#include "ubuntubzr.h"

#include <projectexplorer/session.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/target.h>
#include <projectexplorer/kitinformation.h>
#include <coreplugin/infobar.h>

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

    connect(m_ui->comboBoxFramework,SIGNAL(currentIndexChanged(int)),this,SLOT(setDirty()));
    connect(m_ui->lineEdit_description,SIGNAL(textChanged(QString)),this,SLOT(setDirty()));
    connect(m_ui->lineEdit_maintainer,SIGNAL(textChanged(QString)),this,SLOT(setDirty()));
    connect(m_ui->lineEdit_name,SIGNAL(textChanged(QString)),this,SLOT(setDirty()));
    connect(m_ui->lineEdit_title,SIGNAL(textChanged(QString)),this,SLOT(setDirty()));
    connect(m_ui->lineEdit_version,SIGNAL(textChanged(QString)),this,SLOT(setDirty()));

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

void UbuntuManifestEditorWidget::bzrChanged()
{
    UbuntuBzr *bzr = UbuntuBzr::instance();

    m_ui->lineEdit_maintainer->setText(bzr->whoami());

    /* Commented out for bug #1219948  - https://bugs.launchpad.net/qtcreator-plugin-ubuntu/+bug/1219948
    QString userName = bzr->launchpadId();
    if (userName.isEmpty()) userName = QLatin1String(Constants::USERNAME);
    m_ui->lineEdit_maintainer->setText(QString(QLatin1String("com.ubuntu.developer.%0.%1")).arg(userName).arg(m_projectName));
    */

    if(activePage() != General)
        syncToSource();
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
    return QString(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_DEFAULT_NAME)).arg(userName).arg(projectName);
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
                const UbuntuClickTool::Target *t = 0;
                ProjectExplorer::Project *p = ProjectExplorer::SessionManager::startupProject();
                if (p)
                    t = UbuntuClickTool::clickTargetFromTarget(p->activeTarget());

                targetObject.insert(i.key(),UbuntuClickTool::getMostRecentFramework( QString(), t));
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

/*
void UbuntuPackagingWidget::updatePolicyForFramework(const QString &fw)
{
    if(fw.isEmpty())
        return;

#if 0
    QProcess proc;
    proc.setProgram(QStringLiteral("aa-clickquery"));
    proc.setArguments(QStringList{
                      QStringLiteral("--click-framework=%1").arg(fw),
                      QStringLiteral("--query=policy_version")});
    proc.start();
    proc.waitForFinished();
    if(proc.exitCode() == 0 && proc.exitStatus() == QProcess::NormalExit) {
        m_apparmor.setPolicyVersion(QString::fromUtf8(proc.readAllStandardOutput()));
        m_apparmor.save();
    }
#else
    static const QMap<QString,QString> policy {
        {QStringLiteral("ubuntu-sdk-13.10"),QStringLiteral("1.0")},
        {QStringLiteral("ubuntu-sdk-14.04-dev1"),QStringLiteral("1.1")},
        {QStringLiteral("ubuntu-sdk-14.04-html-dev1"),QStringLiteral("1.1")},
        {QStringLiteral("ubuntu-sdk-14.04-html"),QStringLiteral("1.1")},
        {QStringLiteral("ubuntu-sdk-14.04-papi-dev1"),QStringLiteral("1.1")},
        {QStringLiteral("ubuntu-sdk-14.04-papi"),QStringLiteral("1.1")},
        {QStringLiteral("ubuntu-sdk-14.04-qml-dev1"),QStringLiteral("1.1")},
        {QStringLiteral("ubuntu-sdk-14.04-qml"),QStringLiteral("1.1")},
        {QStringLiteral("ubuntu-sdk-14.04"),QStringLiteral("1.1")},
        {QStringLiteral("ubuntu-sdk-14.10-dev1"),QStringLiteral("1.2")},
        {QStringLiteral("ubuntu-sdk-14.10-dev2"),QStringLiteral("1.2")},
        {QStringLiteral("ubuntu-sdk-14.10-html-dev1"),QStringLiteral("1.2")},
        {QStringLiteral("ubuntu-sdk-14.10-html-dev2"),QStringLiteral("1.2")},
        {QStringLiteral("ubuntu-sdk-14.10-papi-dev1"),QStringLiteral("1.2")},
        {QStringLiteral("ubuntu-sdk-14.10-papi-dev2"),QStringLiteral("1.2")},
        {QStringLiteral("ubuntu-sdk-14.10-qml-dev1"),QStringLiteral("1.2")},
        {QStringLiteral("ubuntu-sdk-14.10-qml-dev2"),QStringLiteral("1.2")},
        {QStringLiteral("ubuntu-sdk-14.10-qml-dev3"),QStringLiteral("1.2")}
    };

    if(policy.contains(fw)) {
        m_apparmor.setPolicyVersion(policy[fw]);
        m_apparmor.save();
    }
#endif
}


*/

} // namespace Internal
} // namespace Ubuntu
