#include <QtTest>
#include <QBuffer>

#include "tst_localportmanager.h"

tst_LocalPortManager::tst_LocalPortManager()
{

}

void tst_LocalPortManager::testEmptyOutput()
{
    QByteArray emptyData;
    QBuffer emptyInput(&emptyData);
    QVERIFY(emptyInput.open(QIODevice::ReadOnly));

    Ubuntu::Internal::UbuntuLocalPortsManager pM;
    pM.setPortsRange(10000,11000);

    int requiredCount = 20;
    Utils::PortList ports = pM.getFreeRange("08866fd9e24cf55a",requiredCount,&emptyInput);


    QCOMPARE(ports.count(),requiredCount);

    int count = 0;
    while(ports.hasMore()) {
        int port = ports.getNext();
        QCOMPARE(port,10000+count);
        count ++;
    }

    QCOMPARE(count,requiredCount);

}

void tst_LocalPortManager::testSimpleList()
{
    QFile in(":/portmanager/simple_list");
    QVERIFY(in.open(QIODevice::ReadOnly));

    int firstPort = 10000;
    int lastPort  = 11000;

    Ubuntu::Internal::UbuntuLocalPortsManager pM;
    pM.setPortsRange(firstPort,lastPort);

    int requiredCount = 20;
    Utils::PortList ports = pM.getFreeRange("08866fd9e24cf55a",requiredCount,&in);
    QCOMPARE(ports.count(),requiredCount);

    int max = firstPort + requiredCount;
    for (int port = firstPort; port < max; ++port) {
        QVERIFY2(ports.contains(port),qPrintable(QStringLiteral("List should contain port: %1").arg(port)));
    }
}


void tst_LocalPortManager::testComplexList()
{
    QFile in(":/portmanager/complex_list");
    QVERIFY(in.open(QIODevice::ReadOnly));

    int firstPort = 10000;
    int lastPort  = 11000;

    Ubuntu::Internal::UbuntuLocalPortsManager pM;
    pM.setPortsRange(firstPort,lastPort);

    int requiredCount = 20;
    Utils::PortList ports = pM.getFreeRange("09977feaf35d066b",requiredCount,&in);
    in.seek(0);

    //the ports should be  0 - 13 and 19 - 24
    for (int i = 0; i <= 13; i++)
        QVERIFY2(ports.contains(firstPort+i),qPrintable(QStringLiteral("List should contain port: %1").arg(firstPort+i)));

    for(int i = 19; i <= 24; i++)
        QVERIFY2(ports.contains(firstPort+i),qPrintable(QStringLiteral("List should contain port: %1").arg(firstPort+i)));

    while(ports.hasMore())
        QVERIFY2(!ports.contains(ports.getNext()),"Ports can not be assigned twice");

    ports = pM.getFreeRange("08866fd9e24cf55a",requiredCount,&in);

    //the ports should be 8 - 18 and 21 - 29
    for (int i = 8; i <= 18; i++)
        QVERIFY2(ports.contains(firstPort+i),qPrintable(QStringLiteral("List should contain port: %1").arg(firstPort+i)));
    for (int i = 21; i <= 29; i++)
        QVERIFY2(ports.contains(firstPort+i),qPrintable(QStringLiteral("List should contain port: %1").arg(firstPort+i)));
    while(ports.hasMore())
        QVERIFY2(!ports.contains(ports.getNext()),"Ports can not be assigned twice");
}

QTEST_GUILESS_MAIN(tst_LocalPortManager)



