/*
 * Copyright 2013 Canonical Ltd.
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
 * Author: Juhapekka Piiroinen <juhapekka.piiroinen@canonical.com>
 */

#include "ubuntuclickmanifest.h"
#include "ubuntuconstants.h"
#include "ubuntuclicktool.h"
#include "ubuntushared.h"

#include <QFile>
#include <QtScriptTools/QScriptEngineDebugger>
#include <QJsonDocument>
#include <QProcess>
#include <QDebug>
#include <QMainWindow>
#include <QAction>
#include <QScriptValueIterator>

#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>
#include <projectexplorer/iprojectmanager.h>
#include <projectexplorer/target.h>

using namespace Ubuntu::Internal;

enum {
    debug = 0
};

UbuntuClickManifest::UbuntuClickManifest(QObject *parent) :
    QObject(parent), m_bInitialized(false), m_bNameDashReplaced(false)

{
    QScriptEngineDebugger debugger;
     debugger.attachTo(&engine);
     debugger.setAutoShowStandardWindow(true);

    QFile manifestAppFile(QLatin1String(":/ubuntu/manifestlib.js"));
    if (!manifestAppFile.open(QIODevice::ReadOnly)) { if(debug) qDebug() << QLatin1String("unable to open js app"); return; }
    QString manifestApp = QString::fromLatin1(manifestAppFile.readAll());
    manifestAppFile.close();

    QScriptProgram program(manifestApp);
    QScriptValue val = engine.evaluate(program);
    if (val.isNull()) { qWarning() << QLatin1String("unable to process app"); return; }

    //load(QLatin1String(":/ubuntu/manifest.json.template"));

}

void UbuntuClickManifest::setName(QString name) {
    if (!isInitialized()) { return; }
    callSetStringFunction(QLatin1String("setName"),name);
    emit nameChanged();
}

QString UbuntuClickManifest::name() {
    return callGetStringFunction(QLatin1String("getName"));
}

void UbuntuClickManifest::setVersion(QString version) {
    if (!isInitialized()) { return; }
    callSetStringFunction(QLatin1String("setVersion"),version);
    emit versionChanged();
}

QString UbuntuClickManifest::version() {
    return callGetStringFunction(QLatin1String("getVersion"));
}

void UbuntuClickManifest::setDescription(QString description) {
    if (!isInitialized()) { return; }
    callSetStringFunction(QLatin1String("setDescription"),description);
    emit descriptionChanged();
}

QString UbuntuClickManifest::description() {
    return callGetStringFunction(QLatin1String("getDescription"));
}

void UbuntuClickManifest::setMaintainer(QString maintainer) {
    if (!isInitialized()) { return; }

    callSetStringFunction(QLatin1String("setMaintainer"),maintainer);
    emit maintainerChanged();
}

QString UbuntuClickManifest::maintainer() {
    if (!isInitialized()) { return QString(); }
    return callGetStringFunction(QLatin1String("getMaintainer"));
}

void UbuntuClickManifest::setTitle(QString title) {
    if (!isInitialized()) { return; }
    callSetStringFunction(QLatin1String("setTitle"),title);
    emit titleChanged();
}

QString UbuntuClickManifest::title() {
    if (!isInitialized()) { return QString(); }
    return callGetStringFunction(QLatin1String("getTitle"));
}

void UbuntuClickManifest::setPolicyVersion(const QString &version) {
    if (!isInitialized()) { return; }

    QStringList args;
    args << version;
    callSetStringListFunction(QLatin1String("setPolicyVersion"),args);
    emit policyVersionChanged();
}

QString UbuntuClickManifest::policyVersion () {
    if (!isInitialized()) { return QString(); }
    return callGetStringFunction(QLatin1String("getPolicyVersion"));
}

void UbuntuClickManifest::setPolicyGroups(QStringList groups) {
    if (!isInitialized()) { return; }

    QStringList args;
    args << groups.join(QLatin1String(" "));
    callSetStringListFunction(QLatin1String("setPolicyGroups"),args);
    emit policyGroupsChanged();

}

QStringList UbuntuClickManifest::policyGroups() {
    if (!isInitialized()) { return QStringList(); }
    QStringList retval = callGetStringListFunction(QLatin1String("getPolicyGroups"));
    return retval;
}

QList<UbuntuClickManifest::Hook> UbuntuClickManifest::hooks()
{
    QList<UbuntuClickManifest::Hook> hooks;
    if (!isInitialized()) { return hooks; }

    QScriptValue scriptHooks = callGetFunction(QLatin1String("getHooks"),QScriptValueList());
    if(!scriptHooks.isObject())
        return hooks;

    QScriptValueIterator it(scriptHooks);
    while (it.hasNext()) {
        it.next();
        QScriptValue appDescriptor = it.value();
        if(!appDescriptor.isObject()) {
            printToOutputPane(tr("Invalid hook in manifest.json file."));
            continue;
        }

        if(!appDescriptor.property(QLatin1String("apparmor")).isValid()) {
            printToOutputPane(tr("The apparmor path is missing in the manifest file"));
            continue;
        }

        bool isScope = appDescriptor.property(QLatin1String("scope")).isValid();
        bool isApp = appDescriptor.property(QLatin1String("desktop")).isValid();

        if( (isScope && isApp) || (!isScope && !isApp)) {
            printToOutputPane(tr("The manifest file needs to specify if this is a app or a scope"));
            continue;
        }

        Hook app;
        app.appId = it.name();
        if(isApp)
            app.desktopFile  = it.value().property(QLatin1String("desktop")).toString();
        if(isScope)
            app.scope  = it.value().property(QLatin1String("scope")).toString();

        app.appArmorFile = it.value().property(QLatin1String("apparmor")).toString();
        hooks.append(app);
    }

    return hooks;
}

void UbuntuClickManifest::setHook(const UbuntuClickManifest::Hook &hook)
{
    Q_UNUSED(hook);

    QScriptValue scriptValue = engine.newObject();

    scriptValue.setProperty(QStringLiteral("appId"),hook.appId);
    scriptValue.setProperty(QStringLiteral("apparmor"),hook.appArmorFile);
    if(!hook.desktopFile.isEmpty()) {
        scriptValue.setProperty(QStringLiteral("desktop"),hook.desktopFile);
    } else if(!hook.appArmorFile.isEmpty()) {
        scriptValue.setProperty(QStringLiteral("scope"),hook.scope);
    } else
        //not known
        return;

    callSetFunction(QStringLiteral("setHook"),QScriptValueList{scriptValue});
}

void UbuntuClickManifest::setFrameworkName(const QString &name)
{
    if (!isInitialized()) { return; }
    callSetStringFunction(QLatin1String("setFrameworkName"),name);
    emit frameworkNameChanged(name);
}

QString UbuntuClickManifest::frameworkName()
{
    if (!isInitialized()) { return QString(); }
    return callGetStringFunction(QLatin1String("getFrameworkName"));
}

QString UbuntuClickManifest::appArmorFileName(const QString &appId)
{
    if (!isInitialized()) { return QString(); }

    QScriptValue v = callGetFunction(QLatin1String("getAppArmorFileName"),QScriptValueList()<<QScriptValue(appId));
    return v.toString();
}

bool UbuntuClickManifest::setAppArmorFileName(const QString &appId, const QString &name)
{
    if (!isInitialized()) { return false; }
    bool result = callFunction(QLatin1String("setAppArmorFileName"),QScriptValueList()<<QScriptValue(appId)<<QScriptValue(name)).toBool();
    callSetFunction(QLatin1String("setAppArmorFileName"), QScriptValueList()<<appId<<name);
    if(result)
        emit appArmorFileNameChanged(appId, name);

    return result;
}

bool UbuntuClickManifest::enableDebugging()
{
    return callFunction(QLatin1String("injectDebugPolicy"),QScriptValueList()).toBool();
}

void UbuntuClickManifest::save(QString fileName) {
    if (!isInitialized()) { return; }

    setFileName(fileName);

    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly)) {
        emit error();
        if(debug) qDebug() << QLatin1String("unable to open file for writing") <<  fileName;
        return;
    }

    QString jsonData = callGetStringFunction(QLatin1String("toJSON"));
    file.write(jsonData.toUtf8());
    file.close();

    emit saved();
}

void UbuntuClickManifest::reload() {
    load(m_fileName);
}

QString UbuntuClickManifest::raw() {
    if (!isInitialized()) { return QString(); }
    return callGetStringFunction(QLatin1String("toJSON"));
}

void UbuntuClickManifest::setRaw(QString data) {
    if (!isInitialized()) { return; }
    callSetStringFunction(QLatin1String("fromJSON"),data);
    emit loaded();
}

bool UbuntuClickManifest::load(const QString &fileName,ProjectExplorer::Project *proj, QString *errorMessage) {

    setFileName(fileName);
    QFile file(fileName);

    if (!file.exists()) {
        emit error();
        if(debug) qDebug() << QLatin1String("file does not exist");
        if(errorMessage) *errorMessage = tr("File does not exist.");
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        emit error();
        if(debug) qDebug() << QLatin1String("unable to open file for reading");
        if(errorMessage) *errorMessage = tr("File can not be opened.");
        return false;
    }

    QString data = QString::fromUtf8(file.readAll());
    file.close();

    if (fileName == QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_DEFAULT_MANIFEST)) {
        if(!proj) {
            if(errorMessage) *errorMessage = tr("Loading the template file requires the Project argument, this is a bug.");
            return false;
        }

        QString mimeType = proj->projectManager()->mimeType();
        QString proName  = proj->projectFilePath();

        bool isUbuntuProject = (mimeType == QLatin1String(Constants::UBUNTUPROJECT_MIMETYPE));
        bool isUbuntuHtmlProject = proName.endsWith(QLatin1String(Constants::UBUNTUHTMLPROJECT_SUFFIX));

        QString defFramework;
        if(isUbuntuProject && isUbuntuHtmlProject) {
            defFramework = UbuntuClickFrameworkProvider::getMostRecentFramework(QLatin1String("html"));

            if(defFramework.isEmpty())
                defFramework = QLatin1String(Constants::UBUNTU_DEFAULT_HTML_FRAMEWORK);
        } else {
            defFramework = UbuntuClickFrameworkProvider::getMostRecentFramework(QLatin1String("qml"));

            if(defFramework.isEmpty())
                defFramework = QLatin1String(Constants::UBUNTU_DEFAULT_QML_FRAMEWORK);
        }
        data.replace(QLatin1String("myFramework"),defFramework);

        QString tmpProjectName = proj->displayName();
        tmpProjectName.replace(QLatin1String("_"),QLatin1String("-"));
        data.replace(QLatin1String("myapp"),tmpProjectName);
        QString original_security_manifest_name = QString(QLatin1String("%0.json")).arg(tmpProjectName);
        QString replaced_security_manifest_name = original_security_manifest_name;
        replaced_security_manifest_name.replace(QLatin1String("_"),QLatin1String("-"));
        data.replace(original_security_manifest_name,replaced_security_manifest_name);
        QString original_desktop_file = QString(QLatin1String("%0.desktop")).arg(tmpProjectName);
        if (m_bNameDashReplaced) {
            original_desktop_file.replace(QLatin1String("-"),QLatin1String("_"));
            data.replace(QString(QLatin1String("%0.desktop")).arg(tmpProjectName),original_desktop_file);
        }
    }

    if(!loadFromString(data)){
        if(errorMessage) *errorMessage = tr("Parsing failed, please check if the syntax is correct.");
        return false;
    }
    return true;
}

bool UbuntuClickManifest::loadFromString(const QString &data)
{
    //@TODO probably return the error message
    QScriptValue ret = callFunction(QStringLiteral("fromJSON"),QScriptValueList{QScriptValue(data)});
    bool success = ret.toBool();
    if(success) {
        m_bInitialized = true;
        emit loaded();
    }

    return success;
}

QScriptValue UbuntuClickManifest::callFunction(QString functionName, QScriptValueList args) {
    QScriptValue global = engine.globalObject();
    QScriptValue cmd = global.property(functionName);
    return cmd.call(QScriptValue(),args);
}

void UbuntuClickManifest::callSetFunction(QString functionName, QScriptValueList args) {
    callFunction(functionName,args);
}

QScriptValue UbuntuClickManifest::callGetFunction(QString functionName, QScriptValueList args) {
    return callFunction(functionName,args);
}

QStringList UbuntuClickManifest::callGetStringListFunction(QString functionName) {
    QScriptValue retval = callFunction(functionName,QScriptValueList());
    return retval.toVariant().toStringList();
}

QString UbuntuClickManifest::callGetStringFunction(QString functionName) {
    QScriptValue retval = callFunction(functionName,QScriptValueList());
    return retval.toVariant().toString();
}

void UbuntuClickManifest::callSetStringListFunction(QString functionName, QStringList args) {
    QScriptValueList vargs;
    foreach (QString arg, args)
        vargs << QScriptValue(arg);
    callSetFunction(functionName,vargs);
}

void UbuntuClickManifest::callSetStringFunction(QString functionName, QString args) {
    QScriptValueList vargs;
    vargs << QScriptValue(args);
    callSetFunction(functionName,vargs);
}

QStringList UbuntuClickManifest::callGetStringListFunction(QString functionName, QString args) {
    QScriptValueList vargs;
    vargs << QScriptValue(args);
    QScriptValue retval = callFunction(functionName,vargs);
    return retval.toVariant().toStringList();
}

/*!
 * \brief UbuntuClickManifest::Hook::type
 * Returns the hook type depending on which fields are not empty
 */
UbuntuClickManifest::Hook::Type UbuntuClickManifest::Hook::type() const
{
    if(!desktopFile.isEmpty())
        return Application;
    else if(!scope.isEmpty())
        return Scope;
    return Invalid;
}
