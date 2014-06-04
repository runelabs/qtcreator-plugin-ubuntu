#include <QString>
#include <QtTest>
#include "tst_ubuntuversiontest.h"
#include <ubuntuversion.h>
#include <QScopedPointer>

using namespace Ubuntu::Internal;

UbuntuVersionTest::UbuntuVersionTest()
{
}

void UbuntuVersionTest::testParse()
{
    QScopedPointer<UbuntuVersion> ver(UbuntuVersion::fromLsbFile(":/ubuntuversion/lsb-release"));
    QVERIFY(!ver.isNull());
    QCOMPARE(ver->id(),QStringLiteral("Ubuntu"));
    QCOMPARE(ver->release(),QStringLiteral("14.10"));
    QCOMPARE(ver->codename(),QStringLiteral("utopic"));
    QCOMPARE(ver->description(),QStringLiteral("\"Ubuntu Utopic Unicorn (development branch)\""));
}


QTEST_GUILESS_MAIN(UbuntuVersionTest)
