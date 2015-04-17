#include <QString>
#include <QtTest>
#include "tst_manifest.h"
#include <ubuntuclickmanifest.h>



using namespace Ubuntu::Internal;

const char MANIFEST_TEMPLATE[] = ":/ubuntu/test/manifest/manifest.json.template";
const char APPARMOR_TEMPLATE[] = ":/ubuntu/test/manifest/myapp.json.template";
const char MANIFEST_FILE[] = "/tmp/manifest.json";


UbuntuManifestTest::UbuntuManifestTest()
{

}

void UbuntuManifestTest::testWriteStringValue(const QString &value, StrWriteFunc write, StrReadFunc read)
{
    UbuntuClickManifest mani;
    QVERIFY( mani.load(MANIFEST_TEMPLATE) );
    mani.setFileName(MANIFEST_FILE);
    mani.save();

    (mani.*write)(value);
    QCOMPARE((mani.*read)(),value);

    QFile::remove(MANIFEST_FILE);
}

void UbuntuManifestTest::testSave(const QString &templateFile)
{
    UbuntuClickManifest mani;
    QVERIFY( mani.load(templateFile) );
    mani.setFileName(MANIFEST_FILE);
    mani.save();

    QVERIFY (QFile::exists(MANIFEST_FILE));

    QFile file(MANIFEST_FILE);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QVERIFY(mani.raw() == QString::fromUtf8(file.readAll()));
    file.close();
    file.remove();
}

void UbuntuManifestTest::testSave()
{
    testSave(MANIFEST_TEMPLATE);
}

void UbuntuManifestTest::testWriteAppArmorName()
{
    UbuntuClickManifest mani;
    QVERIFY( mani.load(MANIFEST_TEMPLATE) );
    mani.setFileName(MANIFEST_FILE);
    mani.save();

    const char AAFILE[] = "some/path/to/myApp.json";
    QVERIFY(mani.setAppArmorFileName("myapp",QString(AAFILE)));
    QCOMPARE(mani.appArmorFileName("myapp"),QString(AAFILE));

    QFile::remove(MANIFEST_FILE);
}

void UbuntuManifestTest::testWriteName()
{
    testWriteStringValue(
                QString("name"),
                &UbuntuClickManifest::setName,
                &UbuntuClickManifest::name);
}

void UbuntuManifestTest::testWriteMaintainer()
{
    testWriteStringValue(
                QString("John Doe"),
                &UbuntuClickManifest::setMaintainer,
                &UbuntuClickManifest::maintainer);
}

void UbuntuManifestTest::testWriteTitle()
{
    testWriteStringValue(
                QString("Completely Awesome Title äüö#$%"),
                &UbuntuClickManifest::setTitle,
                &UbuntuClickManifest::title);
}

void UbuntuManifestTest::testWriteVersion()
{
    testWriteStringValue(
                QString("1.2.0-test1~ubuntu2"),
                &UbuntuClickManifest::setVersion,
                &UbuntuClickManifest::version);
}

void UbuntuManifestTest::testWriteDescription()
{
    testWriteStringValue(
                QString("This is a application description äöüöl?=)(/&%$§\"!\"§$%&/()=>><<-_.,\\*'+#~’^°"),
                &UbuntuClickManifest::setDescription,
                &UbuntuClickManifest::description);
}

void UbuntuManifestTest::testWriteFrameworkName()
{
    UbuntuClickManifest mani;
    QVERIFY( mani.load(MANIFEST_TEMPLATE) );
    mani.setFileName(MANIFEST_FILE);
    mani.save();

    QString value = "ubuntu-sdk-14.10";
    mani.setFrameworkName(value);
    QCOMPARE(mani.frameworkName(),value);

    QFile::remove(MANIFEST_FILE);
}

void UbuntuManifestTest::testReadHooks()
{
    UbuntuClickManifest mani;
    QVERIFY( mani.load(MANIFEST_TEMPLATE) );
    mani.setFileName(MANIFEST_FILE);
    mani.save();

    /*
        "myapp": {
            "apparmor": "myapp.json",
            "desktop": "myapp.desktop"
        },
        "myapp2": {
            "apparmor": "myapp2.json",
            "desktop": "myapp2.desktop"
        },
        "myscope": {
            "apparmor": "myscope.json",
            "scope": "myscope.ini"
        }
     */

    QList<UbuntuClickManifest::Hook> hooks = mani.hooks();
    QCOMPARE(hooks.size(),3);
    QCOMPARE(hooks[0].appId,QString("myapp"));
    QCOMPARE(hooks[0].appArmorFile,QString("myapp.json"));
    QCOMPARE(hooks[0].desktopFile ,QString("myapp.desktop"));
    QCOMPARE(hooks[0].scope ,QString());

    QCOMPARE(hooks[1].appId,QString("myapp2"));
    QCOMPARE(hooks[1].appArmorFile,QString("myapp2.json"));
    QCOMPARE(hooks[1].desktopFile ,QString("myapp2.desktop"));
    QCOMPARE(hooks[1].scope ,QString());

    QCOMPARE(hooks[2].appId,QString("myscope"));
    QCOMPARE(hooks[2].appArmorFile,QString("myscope.json"));
    QCOMPARE(hooks[2].desktopFile ,QString());
    QCOMPARE(hooks[2].scope ,QString("myscope.ini"));

    QFile::remove(MANIFEST_FILE);
}

void UbuntuManifestTest::testAppArmorFile()
{
    testSave(APPARMOR_TEMPLATE);
}


void UbuntuManifestTest::testWritePolicyVersion ()
{
#ifdef Q_PROCESSOR_POWER
    return;
#endif

    UbuntuClickManifest mani;
    QVERIFY( mani.load(APPARMOR_TEMPLATE) );
    mani.setFileName(MANIFEST_FILE);
    mani.save();

    QString value = "1.2";
    mani.setPolicyVersion(value);
    QCOMPARE(mani.policyVersion(),value);

    value = "0.1";
    mani.setPolicyVersion(value);
    QCOMPARE(mani.policyVersion(),value);

    value = "1.0";
    mani.setPolicyVersion(value);
    QCOMPARE(mani.policyVersion(),value);

    QFile::remove(MANIFEST_FILE);
}

void UbuntuManifestTest::testWritePolicyGroups()
{
    UbuntuClickManifest mani;
    QVERIFY( mani.load(APPARMOR_TEMPLATE) );
    mani.setFileName(MANIFEST_FILE);
    mani.save();


    /*
    "policy_groups": [
        "networking",
        "someOtherGroup",
        "lastGroup"
    ]
    */

    QStringList polGroups = mani.policyGroups();
    QCOMPARE(polGroups.size(),3);
    QCOMPARE(polGroups[0],QString("networking"));
    QCOMPARE(polGroups[1],QString("someOtherGroup"));
    QCOMPARE(polGroups[2],QString("lastGroup"));

    polGroups.append("addedGroup1");
    polGroups.append("addedGroup2");

    mani.setPolicyGroups(polGroups);

    polGroups = mani.policyGroups();
    QCOMPARE(polGroups.size(),5);
    QCOMPARE(polGroups[0],QString("networking"));
    QCOMPARE(polGroups[1],QString("someOtherGroup"));
    QCOMPARE(polGroups[2],QString("lastGroup"));
    QCOMPARE(polGroups[3],QString("addedGroup1"));
    QCOMPARE(polGroups[4],QString("addedGroup2"));

    QFile::remove(MANIFEST_FILE);
}

/*!
 * \brief printToOutputPane
 * Dummy function
 */
void printToOutputPane(const QString &msg)
{
    Q_UNUSED(msg);
}

QTEST_GUILESS_MAIN(UbuntuManifestTest)
