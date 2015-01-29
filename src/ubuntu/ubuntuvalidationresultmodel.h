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
 * Author: Benjamin Zeller <benjamin.zeller@canonical.com>
 */

#ifndef UBUNTU_INTERNAL_UBUNTUVALIDATIONRESULTMODEL_H
#define UBUNTU_INTERNAL_UBUNTUVALIDATIONRESULTMODEL_H

#include <QAbstractItemModel>
#include <QMetaType>
#include <QSharedPointer>
#include <QUrl>

namespace Ubuntu {
namespace Internal {

class ClickRunChecksParser : public QObject
{
    Q_OBJECT

public:
    enum ItemIcon {
        Error,
        Warning,
        Check,
        NoIcon
    };

    struct DataItem {
        DataItem();
        ~DataItem();

        DataItem* parent;

        QString type;
        QString text;
        QUrl link;
        ItemIcon icon;

        QList<DataItem*> children;
    };

    ClickRunChecksParser(QObject* parent = 0);
    void beginRecieveData(const QString& data = QString());
    void addRecievedData (const QString& data);
    void endRecieveData  (const QString& data = QString());

    void emitTextItem (const QString &type, const QString &text, const ItemIcon icon = NoIcon);
protected:
    bool tryParseNextSection ( bool dataComplete = false);
    void emitParseErrorItem  (const QString &text);
    void parseJsonSection (const QString &sectionName, int offset, int length = -1);

signals:
    void parsedNewTopLevelItem (DataItem* item);
    void begin ();
    void finished ();
private:
    QString m_data;
    int     m_nextSectionOffset;
    int     m_errorCount;
    int     m_warnCount;
};

class UbuntuValidationResultModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Roles {
        TypeRole = Qt::UserRole+1,
        LinkRole,
        DescriptionRole,
        ImageRole,
        ShouldExpandRole
    };

    UbuntuValidationResultModel(QObject *parent = 0);
    ~UbuntuValidationResultModel();

    // QAbstractItemModel interface
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual int rowCount(const QModelIndex &parent) const;
    virtual int columnCount(const QModelIndex &parent) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;

    QModelIndex findFirstErrorItem () const;

public slots:
    void appendItem  ( ClickRunChecksParser::DataItem* item );
    void clear ();

private:
    ClickRunChecksParser::DataItem *getItem(const QModelIndex &index) const;

private:
    ClickRunChecksParser::DataItem* m_rootItem;
    QString m_data;
    int     m_nextSectionOffset;
};

} // namespace Internal
} // namespace Ubuntu

Q_DECLARE_METATYPE(Ubuntu::Internal::ClickRunChecksParser::DataItem*)

#endif // UBUNTU_INTERNAL_UBUNTUVALIDATIONRESULTMODEL_H
