#ifndef TST_VALIDATION_H
#define TST_VALIDATION_H

#include <QObject>
#include "ubuntuvalidationresultmodel.h"

class QEventLoop;

class tst_Validation : public QObject
{
    Q_OBJECT

public:
    tst_Validation();

private slots:
    void testSimpleSectionParse ();

private:
    Ubuntu::Internal::ClickRunChecksParser *m_parser;
    QEventLoop* m_loop;

};

#endif // TST_VALIDATION_H
