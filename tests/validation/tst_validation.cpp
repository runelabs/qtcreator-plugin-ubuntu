#include "tst_validation.h"
#include <QtTest>
#include <QTimer>
#include <QDebug>
#include <QFile>
#include <QTextStream>

using namespace Ubuntu::Internal;

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
    //and the parser always waits for end, or begin of the next section
    QVERIFY(items.isEmpty());

    parser.endRecieveData();

    QVERIFY(items.length() == 1);
    QCOMPARE(items[0]->type,QStringLiteral("click-check-functional"));
    QCOMPARE(items[0]->text,QStringLiteral("No description"));
    QCOMPARE(items[0]->icon,ClickRunChecksParser::Check);
    QCOMPARE(items[0]->children.size(),3);

    qDeleteAll(items.begin(),items.end());
}

QTEST_MAIN(tst_Validation)
