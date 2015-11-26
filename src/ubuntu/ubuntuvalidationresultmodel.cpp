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

#include "ubuntuvalidationresultmodel.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>
#include <QDebug>
#include <QStringList>
#include <QVariant>
#include <QIcon>
#include <QDir>


namespace Ubuntu {
namespace Internal {
/*!
 * \class UbuntuValidationResultModel
 * Implements a model to show the output of the click-run-checks parser
 */
UbuntuValidationResultModel::UbuntuValidationResultModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_rootItem(new ClickRunChecksParser::DataItem())
    , m_nextSectionOffset(0)
{
    m_rootItem->type = QLatin1String("RootItem");
}

UbuntuValidationResultModel::~UbuntuValidationResultModel()
{
    delete m_rootItem;
}

void UbuntuValidationResultModel::clear()
{
    beginRemoveRows(QModelIndex(),0,m_rootItem->children.size()-1);
    delete m_rootItem;
    m_rootItem = new ClickRunChecksParser::DataItem();
    m_rootItem->type = QLatin1String("RootItem");
    endRemoveRows();
}

QModelIndex UbuntuValidationResultModel::index(int row, int column, const QModelIndex &parent) const
{
    ClickRunChecksParser::DataItem* item = getItem(parent);
    if(!item) {
        return QModelIndex();
    }

    if(row < 0 || row > item->children.length()-1 || column != 0){
        return QModelIndex();
    }

    ClickRunChecksParser::DataItem* indexItem = item->children[row];
    return createIndex(row,column,indexItem);
}

QModelIndex UbuntuValidationResultModel::parent(const QModelIndex &child) const
{
    if(!child.isValid())
        return QModelIndex();

    ClickRunChecksParser::DataItem* item = getItem(child);
    ClickRunChecksParser::DataItem* parentItem = item->parent;

    if(!parentItem || parentItem == m_rootItem)
        return QModelIndex();

    ClickRunChecksParser::DataItem* ppItem = parentItem->parent;
    if(!ppItem)
        return QModelIndex();

    int row = ppItem->children.indexOf(parentItem);
    Q_ASSERT_X(row >= 0,Q_FUNC_INFO,"Child must be a element of parent list");

    return createIndex(row,0,parentItem);
}

int UbuntuValidationResultModel::rowCount(const QModelIndex &parent) const
{
    int rC = 1;
    ClickRunChecksParser::DataItem* item = getItem(parent);
    if(item)
        rC = item->children.size();

    return rC;
}

int UbuntuValidationResultModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant UbuntuValidationResultModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    ClickRunChecksParser::DataItem* item = getItem(index);
    if(item == m_rootItem)
        return QVariant();

    switch(role) {
        case Qt::DisplayRole:
        case Qt::EditRole: {
            QString text = item->type;
            return text;
            break;
        }
        case TypeRole: {
            return item->type;
            break;
        }
        case DescriptionRole: {
            return item->text;
            break;
        }
        case LinkRole: {
            return item->link.toString();
            break;
        }
        case ImageRole:
        case Qt::DecorationRole: {
            QString iconUrl;
            switch(item->icon){
                case ClickRunChecksParser::Warning:
                    iconUrl = QStringLiteral(":/ubuntu/images/warning.png");
                    break;
                case ClickRunChecksParser::Error:
                    iconUrl = QStringLiteral(":/ubuntu/images/error.png");
                    break;
                case ClickRunChecksParser::Check:
                    iconUrl = QStringLiteral(":/ubuntu/images/run.png");
                    break;
                default:
                    break;
            }

            if(!iconUrl.isEmpty()) {
                if(role == ImageRole) return QString::fromLatin1("qrc%1").arg(iconUrl);
                else return QIcon(iconUrl);
            }

            break;
        }
        case ShouldExpandRole: {
            if(item->icon == ClickRunChecksParser::Error || item->icon == ClickRunChecksParser::Warning)
                return true;
            return false;
        }
    }
    return QVariant();
}

Qt::ItemFlags UbuntuValidationResultModel::flags(const QModelIndex &index) const
{
    return QAbstractItemModel::flags(index);
}

QHash<int, QByteArray> UbuntuValidationResultModel::roleNames() const
{
    QHash<int, QByteArray> roleNames = QAbstractItemModel::roleNames();

    roleNames[TypeRole] = "TypeRole";
    roleNames[LinkRole] = "LinkRole";
    roleNames[DescriptionRole] = "DescriptionRole";
    roleNames[ImageRole] = "ImageRole";
    roleNames[ShouldExpandRole] = "ShouldExpandRole";

    return roleNames;
}

QModelIndex UbuntuValidationResultModel::findFirstErrorItem() const
{
    int parentRow = 0;
    foreach(const ClickRunChecksParser::DataItem* parentItem, m_rootItem->children) {
        int innerRow = 0;
        bool found = false;
        foreach(const ClickRunChecksParser::DataItem* item, parentItem->children) {
            if(item->icon == ClickRunChecksParser::Error || item->icon == ClickRunChecksParser::Warning) {
                found = true;
                break;
            }
            innerRow++;
        }

        if(found) {
            QModelIndex idx = index(innerRow,0,index(parentRow,0,QModelIndex()));
            return idx;
        }
        parentRow++;
    }
    return QModelIndex();
}

void UbuntuValidationResultModel::appendItem(ClickRunChecksParser::DataItem *item)
{
    beginInsertRows(QModelIndex(),m_rootItem->children.length(),m_rootItem->children.length());
    item->parent = m_rootItem;
    m_rootItem->children.append(item);
    endInsertRows();
}

ClickRunChecksParser::DataItem *UbuntuValidationResultModel::getItem(const QModelIndex &index) const
{
    if(index.isValid()){
        ClickRunChecksParser::DataItem* elem = static_cast<ClickRunChecksParser::DataItem*>(index.internalPointer());
        if(elem) {
            return elem;
        }
    }
    return m_rootItem;
}


ClickRunChecksParser::DataItem::DataItem()
    : parent(0)
    , icon(ClickRunChecksParser::NoIcon)
{

}

ClickRunChecksParser::DataItem::~DataItem()
{
    qDeleteAll(children.begin(),children.end());
    children.clear();
}


/*!
 * \class ClickRunChecksParser
 * Implements a incremental parser for the click-run-checks output
 * As soon as a complete section is available (marked by start of
 * a new one) the section is parsed and a new item is emitted
 */
ClickRunChecksParser::ClickRunChecksParser(QObject *parent)
    : QObject(parent)
    , m_nextSectionOffset(0)
    , m_errorCount(0)
    , m_warnCount(0)
{

}


/*!
 * \brief ClickRunChecksParser::beginRecieveData
 * Clear alls internal data and starts a completely new parse
 */
void ClickRunChecksParser::beginRecieveData(const QString &data)
{
    m_data.clear();
    m_nextSectionOffset = m_errorCount = m_warnCount = 0;
    m_data.append(data);

    emit begin();

    bool canContinue = true;
    while(canContinue) canContinue=tryParseNextSection();
}

/*!
 * \brief ClickRunChecksParser::addRecievedData
 * Appends new \a data to the internal buffer and tries to read
 * as far as possible and emit new items, the parser always requires
 * to find a new section start before considering the current section
 * to be available completely
 */
void ClickRunChecksParser::addRecievedData(const QString &data)
{
    m_data.append(data);

    bool canContinue = true;
    while(canContinue) canContinue=tryParseNextSection();
}

/*!
 * \brief ClickRunChecksParser::endRecieveData
 * Tells the parser every data is available now and it should just
 * parse until the end of the internal buffer
 */
void ClickRunChecksParser::endRecieveData(const QString &data)
{
    m_data.append(data);

    bool canContinue = true;
    while(canContinue) canContinue=tryParseNextSection(true);
    emit finished();
}

/*!
 * \brief UbuntuValidationResultModel::tryParseNextSection
 * Tries to parse the next output section from the dataString
 *
 * \return false if there is no next section
 */
bool ClickRunChecksParser::tryParseNextSection(bool dataComplete)
{
    QRegularExpression re(QStringLiteral("^([=]+[\\s]+[A-Za-z0-9\\-_]+[\\s]+[=]+)$")); // match section beginnings
    re.setPatternOptions(QRegularExpression::MultilineOption);

    QRegularExpressionMatchIterator matchIter = re.globalMatch(m_data,m_nextSectionOffset);
    if(!matchIter.hasNext())
        return false;

    QRegularExpressionMatch match = matchIter.next();
    int startOffset = match.capturedStart(1) + match.capturedLength(1);

    int endOffset   = -1;
    if(matchIter.hasNext()) {
        QRegularExpressionMatch match = matchIter.next();
        endOffset = match.capturedStart(1);
    }

    if(endOffset < 0 && dataComplete)
        endOffset = m_data.length()-1;

    if(startOffset == -1 || endOffset == -1)
        return false;

    //set the offset for the next parse
    m_nextSectionOffset = endOffset;

    QString type = match.captured(1);
    type.remove(QLatin1String("="));
    type = type.trimmed();

    //prior to 5.4.0 we had to add +1 to fix the offset
#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
    parseJsonSection(type,startOffset,(endOffset-startOffset));
#else
    parseJsonSection(type,startOffset,(endOffset-startOffset)+1);
#endif
    return true;
}

/*!
 * \brief ClickRunChecksParser::parseJsonSection
 * Parses a part of the input data, that can contain only a JSON object
 * or alternatively a single line starting with the string ERROR:
 * emits parsedNewItem if the section was parsed successfully
 */
void ClickRunChecksParser::parseJsonSection(const QString &sectionName, int offset, int length)
{
    QJsonParseError error;

    QString sectionData = m_data.mid(offset,length);
    sectionData = sectionData.trimmed();

    if(sectionData.startsWith(QLatin1String("ERROR:"))) {
        QStringList lines = sectionData.split(QLatin1String("\n"));
        QString errLine = lines.size() > 0 ? lines[0] : sectionData;

        errLine.remove(QLatin1String("ERROR:"));
        emitTextItem(sectionName,errLine,Error);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(sectionData.toUtf8(),&error);

    if(error.error != QJsonParseError::NoError) {
        emitParseErrorItem(QString::fromLocal8Bit("Json Parse Error: %0").arg(error.errorString()));
        return;
    }

    if(!doc.isObject()) {
        emitParseErrorItem(QString::fromLocal8Bit("Json unexpected format"));
        return;
    }

    bool hasErrors = false;
    bool hasWarnings = false;
    DataItem* item = new DataItem();
    item->text = QLatin1String("No description");
    item->type = sectionName;
    item->icon = Check; // we are optimisic

    QJsonObject obj = doc.object();

    QJsonValue errValue = obj.value(QLatin1String("error"));
    QJsonValue warnValue = obj.value(QLatin1String("warn"));
    QJsonValue infoValue = obj.value(QLatin1String("info"));

    if(errValue.isObject()) {
        QJsonObject errors = errValue.toObject();
        QStringList keys = errors.keys();
        hasErrors = (errors.keys().size() > 0);
        foreach(const QString &key, keys) {
            QJsonValue messageValue = errors.value(key);
            if(messageValue.isObject()){
                QJsonObject messageObject = messageValue.toObject();

                QString text = messageObject.value(QLatin1String("text")).toString();
                DataItem* subItem = new DataItem();
                subItem->parent = item;
                subItem->text = text;
                subItem->type = key;
                subItem->icon = Error;

                if(messageObject.keys().contains(QLatin1String("link"))) {
                    subItem->link = QUrl::fromUserInput(messageObject.value(QLatin1String("link")).toString());
                }

                item->children.append(subItem);
            }
        }
    }

    if(warnValue.isObject()) {
        QJsonObject warnings = warnValue.toObject();
        QStringList keys = warnings.keys();
        hasWarnings = (warnings.keys().size() > 0);
        foreach(const QString &key, keys) {
            QJsonValue messageValue = warnings.value(key);
            if(messageValue.isObject()){
                QJsonObject messageObject = messageValue.toObject();

                QString text = messageObject.value(QLatin1String("text")).toString();
                DataItem* subItem = new DataItem();
                subItem->parent = item;
                subItem->text = text;
                subItem->type = key;
                subItem->icon = Warning;

                if(messageObject.keys().contains(QLatin1String("link"))) {
                    subItem->link = QUrl::fromUserInput(messageObject.value(QLatin1String("link")).toString());
                }

                item->children.append(subItem);
            }
        }
    }

    if(infoValue.isObject()) {
        QJsonObject infos = infoValue.toObject();
        QStringList keys = infos.keys();
        foreach(const QString &key, keys) {
            QJsonValue messageValue = infos.value(key);
            if(messageValue.isObject()){
                QJsonObject messageObject = messageValue.toObject();

                QString text = messageObject.value(QLatin1String("text")).toString();
                DataItem* subItem = new DataItem();
                subItem->parent = item;
                subItem->type = key;
                subItem->text = text;
                subItem->icon = Check;

                if(messageObject.keys().contains(QLatin1String("link"))) {
                    subItem->link = QUrl::fromUserInput(messageObject.value(QLatin1String("link")).toString());
                }

                item->children.append(subItem);
            }
        }
    }

    if(hasErrors)
        item->icon = Error;
    else if(hasWarnings)
        item->icon = Warning;

    emit parsedNewTopLevelItem(item);
}

/*!
 * \brief ClickRunChecksParser::emitParseErrorItem
 * Creates a new Error DataItem and emits it
 */
void ClickRunChecksParser::emitParseErrorItem(const QString &text)
{
    DataItem *elem = new DataItem();
    elem->icon = Error;
    elem->type = QLatin1String("Error");
    elem->text = text;

    emit parsedNewTopLevelItem(elem);
}

/*!
 * \brief ClickRunChecksParser::emitTextItem
 * Creates a new text item and emits it
 */
void ClickRunChecksParser::emitTextItem(const QString& type,const QString &text, const ItemIcon icon)
{
    DataItem *elem = new DataItem();
    elem->icon = icon;
    elem->type = type;
    elem->text = text;

    emit parsedNewTopLevelItem(elem);
}

} // namespace Internal
} // namespace Ubuntu
