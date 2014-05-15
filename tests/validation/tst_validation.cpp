#include "tst_validation.h"
#include <QtTest>
#include <QTimer>
#include <QDebug>
#include <QFile>
#include <QTextStream>

using namespace Ubuntu::Internal;

#define VERIFY_ITEM(item,itemType,itemText,itemIcon,cCount) \
    do { \
    QCOMPARE(item->type,itemType); \
    QCOMPARE(item->text,itemText); \
    QCOMPARE(item->icon,itemIcon); \
    QCOMPARE(item->children.size(),cCount); \
    }while(0)

tst_Validation::tst_Validation() :
    m_parser(0) ,
    m_loop(new QEventLoop(this))
{

}

void tst_Validation::testSimpleSectionParse()
{
    QList<ClickRunChecksParser::DataItem*> items;
    ClickRunChecksParser parser;

    connect(&parser,&ClickRunChecksParser::parsedNewTopLevelItem,
            [this,&items](ClickRunChecksParser::DataItem *newItem){
        items.append(newItem);
    });

    QFile sourceFile(":/validation/simplesection.json");
    QVERIFY(sourceFile.open(QIODevice::ReadOnly));

    QTextStream in(&sourceFile);
    parser.beginRecieveData(in.readAll());

    //until now no item can be available because we have only one section
    //and the parser always waits for end of the document, or begin of the next section
    QVERIFY(items.isEmpty());

    parser.endRecieveData();

    QVERIFY(items.length() == 1);
    VERIFY_ITEM(items[0],QString("click-check-functional"),QString("No description"),ClickRunChecksParser::Check,3);
    VERIFY_ITEM(items[0]->children[0],QString("test1"),QString("OK"),ClickRunChecksParser::Check,0);
    VERIFY_ITEM(items[0]->children[1],QString("test2"),QString("OK"),ClickRunChecksParser::Check,0);
    VERIFY_ITEM(items[0]->children[2],QString("test3"),QString("OK"),ClickRunChecksParser::Check,0);

    qDeleteAll(items.begin(),items.end());
}

QTEST_MAIN(tst_Validation)
