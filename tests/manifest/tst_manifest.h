#ifndef TST_MANIFEST_H
#define TST_MANIFEST_H

#include <QObject>
#include <ubuntuclickmanifest.h>

typedef void    (Ubuntu::Internal::UbuntuClickManifest::*StrWriteFunc) (QString arg);
typedef QString (Ubuntu::Internal::UbuntuClickManifest::*StrReadFunc)  ( );

class UbuntuManifestTest : public QObject
{
    Q_OBJECT

public:
    UbuntuManifestTest();

private:
    void testWriteStringValue (const QString &value, StrWriteFunc write, StrReadFunc read);
    void testSave (const QString &templateFile);

private slots:
    void testSave ();
    void testWriteAppArmorName ();
    void testWriteName ();
    void testAppArmorFile ();

    void testWriteMaintainer();
    void testWriteTitle();
    void testWriteVersion();
    void testWriteDescription();
    void testWriteFrameworkName();
    void testReadHooks();
    void testWritePolicyVersion();
    void testWritePolicyGroups();
};

#endif // TST_MANIFEST_H
