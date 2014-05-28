#ifndef TST_UBUNTUVERSIONTEST_H
#define TST_UBUNTUVERSIONTEST_H

#include <QObject>

class UbuntuVersionTest : public QObject
{
    Q_OBJECT

public:
    UbuntuVersionTest();

private slots:
    void testParse ();

};

#endif // TST_UBUNTUVERSIONTEST_H
