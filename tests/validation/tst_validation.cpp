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

/*!
 * \brief tst_Validation::testSimpleSectionParse
 * Tries to parse a simple section
 */
void tst_Validation::testSimpleSectionParse()
{
    QList<ClickRunChecksParser::DataItem*> items;
    ClickRunChecksParser parser;

    auto appendItemCallback = [this,&items](ClickRunChecksParser::DataItem *newItem){
        items.append(newItem);
    };

    connect(&parser,&ClickRunChecksParser::parsedNewTopLevelItem,appendItemCallback);

    QFile sourceFile(":/validation/simplesection.json");
    QVERIFY(sourceFile.open(QIODevice::ReadOnly));

    QTextStream in(&sourceFile);
    parser.beginRecieveData(in.readAll());

    //until now no item can be available because we have only one section
    //and the parser always waits for end of the document, or begin of the next section
    QVERIFY(items.isEmpty());

    parser.endRecieveData();

    QVERIFY(items.length() == 1);
    VERIFY_ITEM(items[0],QString("functional"),QString("No description"),ClickRunChecksParser::Error,5);
    VERIFY_ITEM(items[0]->children[0],QString("error1"),QString("Error text"),ClickRunChecksParser::Error,0);
    QCOMPARE(items[0]->children[0]->link,QUrl("http://somelink.com"));
    VERIFY_ITEM(items[0]->children[1],QString("warning1"),QString("Warning message"),ClickRunChecksParser::Warning,0);
    QCOMPARE(items[0]->children[1]->link,QUrl("http://somelink.com/warning"));
    VERIFY_ITEM(items[0]->children[2],QString("test1"),QString("OK"),ClickRunChecksParser::Check,0);
    VERIFY_ITEM(items[0]->children[3],QString("test2"),QString("OK"),ClickRunChecksParser::Check,0);
    VERIFY_ITEM(items[0]->children[4],QString("test3"),QString("OK"),ClickRunChecksParser::Check,0);

    qDeleteAll(items.begin(),items.end());
}


void tst_Validation::testFullOutputLint(ClickRunChecksParser::DataItem *item, bool *passed)
{
    VERIFY_ITEM(item,QString("lint"),QString("No description"),ClickRunChecksParser::Error,4);
    VERIFY_ITEM(item->children[0],QString("lint_error"),QString("errorText"),ClickRunChecksParser::Error,0);
    VERIFY_ITEM(item->children[1],QString("lint_info1"),QString("OK"),ClickRunChecksParser::Check,0);
    VERIFY_ITEM(item->children[2],QString("lint_info2"),QString("OK"),ClickRunChecksParser::Check,0);
    VERIFY_ITEM(item->children[3],QString("lint_info3"),QString("OK"),ClickRunChecksParser::Check,0);
    *passed=true;
}

void tst_Validation::testFullOutputDesktop(ClickRunChecksParser::DataItem *item, bool *passed)
{
    VERIFY_ITEM(item,QString("desktop"),QString("No description"),ClickRunChecksParser::Error,3);
    VERIFY_ITEM(item->children[0],QString("desktop_error (test1)"),QString("Error text for desktop file"),ClickRunChecksParser::Error,0);
    QCOMPARE(item->children[0]->link,QUrl("http://somelink.com"));
    VERIFY_ITEM(item->children[1],QString("desktop_warn (test1)"),QString("Warning text for desktop file"),ClickRunChecksParser::Warning,0);
    QCOMPARE(item->children[1]->link,QUrl("http://somelink.com/warning"));
    VERIFY_ITEM(item->children[2],QString("desktop_info (test1)"),QString("OK"),ClickRunChecksParser::Check,0);
    *passed=true;
}

void tst_Validation::testFullOutputSecurity(ClickRunChecksParser::DataItem *item, bool *passed)
{
    VERIFY_ITEM(item,QString("security"),QString("No description"),ClickRunChecksParser::Check,3);
    VERIFY_ITEM(item->children[0],QString("security_test1 (test.json)"),QString("OK"),ClickRunChecksParser::Check,0);
    VERIFY_ITEM(item->children[1],QString("security_test2 (test.json)"),QString("OK"),ClickRunChecksParser::Check,0);
    VERIFY_ITEM(item->children[2],QString("security_test3 (test.json)"),QString("OK"),ClickRunChecksParser::Check,0);
    *passed=true;
}

void tst_Validation::testFullOutputFunctional(ClickRunChecksParser::DataItem *item, bool *passed)
{
    VERIFY_ITEM(item,QString("functional"),QString("No description"),ClickRunChecksParser::Check,3);
    VERIFY_ITEM(item->children[0],QString("functional_test1"),QString("OK"),ClickRunChecksParser::Check,0);
    VERIFY_ITEM(item->children[1],QString("functional_test2"),QString("OK"),ClickRunChecksParser::Check,0);
    VERIFY_ITEM(item->children[2],QString("functional_test3"),QString("OK"),ClickRunChecksParser::Check,0);
    *passed=true;
}

/*!
 * \brief tst_Validation::testCompleteOutput
 * Tests to parse a complete output from click-run-checks
 */
void tst_Validation::testCompleteOutput()
{
    QList<ClickRunChecksParser::DataItem*> items;
    ClickRunChecksParser parser;

    connect(&parser,&ClickRunChecksParser::parsedNewTopLevelItem,
            [this,&items](ClickRunChecksParser::DataItem *newItem){
        items.append(newItem);
    });

    QFile sourceFile(":/validation/fulloutput.json");
    QVERIFY(sourceFile.open(QIODevice::ReadOnly));

    QTextStream in(&sourceFile);
    parser.beginRecieveData(in.readAll());
    //until now only 3 of the 4 items can be available because we have four sections
    //and the parser always waits for end of the document, or begin of the next section
    QCOMPARE(items.length(),3);

    parser.endRecieveData();

    QCOMPARE(items.length(),4);

    bool passed = false;
    testFullOutputLint(items[0],&passed);
    if(!passed) return;

    passed = false;
    testFullOutputDesktop(items[1],&passed);
    if(!passed) return;

    passed = false;
    testFullOutputSecurity(items[2],&passed);
    if(!passed) return;

    passed = false;
    testFullOutputFunctional(items[3],&passed);
    if(!passed) return;


    qDeleteAll(items.begin(),items.end());
}

/**
 * @brief tst_Validation::testIncrementalParse
 * Tests the incremental parsing of ClickRunChecksParser,
 * reads the fulloutput.json file and splits it up in chunks
 */
void tst_Validation::testIncrementalParse()
{
    QList<ClickRunChecksParser::DataItem*> items;
    ClickRunChecksParser parser;

    connect(&parser,&ClickRunChecksParser::parsedNewTopLevelItem,
            [this,&items](ClickRunChecksParser::DataItem *newItem){
        items.append(newItem);
    });

    QFile sourceFile(":/validation/fulloutput.json");
    QVERIFY(sourceFile.open(QIODevice::ReadOnly));

    QTextStream in(&sourceFile);
    QString input = in.readAll();

    //parse to somewhere in the middle of a section
    int offset = input.indexOf("info",input.indexOf("= desktop ="));
    parser.addRecievedData(input.left(offset));
    input = input.mid(offset);

    //one section should be available now
    QCOMPARE(items.length(),1);
    bool passed = false;
    testFullOutputLint(items[0],&passed);
    if(!passed) return;

    /*
     * Try to parse more than one section,
     * Add the beginning of the next section so the
     * parser knows where the last one ends
     */
    QString functionalSectionStart("= functional =");
    offset = input.indexOf(functionalSectionStart)+functionalSectionStart.length();
    parser.addRecievedData(input.left(offset));
    input = input.mid(offset);

    //there should be 3 sections now
    QCOMPARE(items.length(),3);
    passed = false;
    testFullOutputDesktop(items[1],&passed);
    if(!passed) return;

    passed = false;
    testFullOutputSecurity(items[2],&passed);
    if(!passed) return;

    //parse the remaining bits
    parser.endRecieveData(input);

    //there should be 4 sections now
    QCOMPARE(items.length(),4);
    passed = false;
    testFullOutputFunctional(items[3],&passed);
    if(!passed) return;

    qDeleteAll(items.begin(),items.end());
}

QTEST_GUILESS_MAIN(tst_Validation)
