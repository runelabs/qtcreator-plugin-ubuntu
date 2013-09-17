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
#include <QFile>
#include <QtScriptTools/QScriptEngineDebugger>
#include <QJsonDocument>
#include <QProcess>
#include <QDebug>
#include <QMainWindow>
#include <QAction>

using namespace Ubuntu::Internal;

UbuntuClickManifest::UbuntuClickManifest(QObject *parent) :
    QObject(parent), m_bInitialized(false), m_bNameDashReplaced(false)

{
    QScriptEngineDebugger debugger;
     debugger.attachTo(&engine);
     debugger.setAutoShowStandardWindow(true);

    QFile manifestAppFile(QLatin1String(":/ubuntu/manifestlib.js"));
    if (!manifestAppFile.open(QIODevice::ReadOnly)) { qDebug() << QLatin1String("unable to open js app"); return; }
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

void UbuntuClickManifest::setPolicyGroups(QString appName, QStringList groups) {
    if (!isInitialized()) { return; }

    QStringList args;
    args << appName << groups.join(QLatin1String(" "));
    callSetStringListFunction(QLatin1String("setPolicyGroups"),args);
    emit policyGroupsChanged();

}

QStringList UbuntuClickManifest::policyGroups(QString appName) {
    if (!isInitialized()) { return QStringList(); }
    QStringList retval = callGetStringListFunction(QLatin1String("getPolicyGroups"),appName);
    return retval;
}

void UbuntuClickManifest::save(QString fileName) {
    if (!isInitialized()) { return; }

    setFileName(fileName);

    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly)) {
        emit error();
        qDebug() << QLatin1String("unable to open file for writing") <<  fileName;
        return;
    }

    QJsonDocument jsonDoc = QJsonDocument::fromJson(callGetStringFunction(QLatin1String("toJSON")).toLatin1());
    file.write(jsonDoc.toJson());

    file.close();

    emit saved();
}

void UbuntuClickManifest::reload() {
    load(m_fileName,m_projectName);
}

QString UbuntuClickManifest::raw() {
    if (!isInitialized()) { return QString(); }
    return QString::fromLatin1(QJsonDocument::fromJson(callGetStringFunction(QLatin1String("toJSON")).toLatin1()).toJson());
}

void UbuntuClickManifest::setRaw(QString data) {
    if (!isInitialized()) { return; }
    callSetStringFunction(QLatin1String("fromJSON"),data);
    emit loaded();
}

void UbuntuClickManifest::load(QString fileName, QString projectName) {
    setFileName(fileName);
    m_projectName = projectName;

    QFile file(fileName);

    if (!file.exists()) {
        emit error();
        qDebug() << QLatin1String("file does not exist");
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        emit error();
        qDebug() << QLatin1String("unable to open file for reading");

        return;
    }

    QString data = QString::fromLatin1(file.readAll());
    file.close();

    if (fileName == QLatin1String(":/ubuntu/manifest.json.template")) {
	projectName.replace(QLatin1String("_"),QLatin1String("-"));
        data.replace(QLatin1String("myapp"),projectName);
	QString original_security_manifest_name = QString(QLatin1String("%0.json")).arg(projectName);
	QString replaced_security_manifest_name = original_security_manifest_name;
	replaced_security_manifest_name.replace(QLatin1String("_"),QLatin1String("-"));
	data.replace(original_security_manifest_name,replaced_security_manifest_name);
	QString original_desktop_file = QString(QLatin1String("%0.desktop")).arg(projectName);
	if (m_bNameDashReplaced) {
	       original_desktop_file.replace(QLatin1String("-"),QLatin1String("_"));
	       data.replace(QString(QLatin1String("%0.desktop")).arg(projectName),original_desktop_file);
	}
    }

    callSetStringFunction(QLatin1String("fromJSON"),data);

    m_bInitialized = true;
    emit loaded();
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

