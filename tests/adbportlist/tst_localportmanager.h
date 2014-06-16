#ifndef TST_LOCALPORTMANAGER_H
#define TST_LOCALPORTMANAGER_H

#include <QObject>
#include "localportsmanager.h"

class tst_LocalPortManager : public QObject
{
    Q_OBJECT

public:
    tst_LocalPortManager();


private slots:
    void testEmptyOutput();
    void testSimpleList ();
    void testComplexList();
};

#endif // TST_LOCALPORTMANAGER_H
