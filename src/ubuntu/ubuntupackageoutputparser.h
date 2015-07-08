#ifndef UBUNTU_INTERNAL_UBUNTUPACKAGEOUTPUTPARSER_H
#define UBUNTU_INTERNAL_UBUNTUPACKAGEOUTPUTPARSER_H

#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/itaskhandler.h>
#include "ubuntuvalidationresultmodel.h"

namespace Ubuntu {
namespace Internal {

class UbuntuPackageOutputParser : public ProjectExplorer::IOutputParser
{
    Q_OBJECT
public:
    explicit UbuntuPackageOutputParser();

public:
    // IOutputParser interface
    virtual void stdOutput(const QString &line);
    virtual void stdError(const QString &line);
    virtual bool hasFatalErrors() const;

    void setTreatAllErrorsAsWarnings ( const bool set );

public slots:
    void setEndOfData ();

private slots:
    void onParsedNewTopLevelItem (ClickRunChecksParser::DataItem* item);

private:
    virtual void doFlush();
    void emitTasks (const ClickRunChecksParser::DataItem *item, int level = 0);
    bool isError (const ClickRunChecksParser::DataItem *item);
    ClickRunChecksParser m_subParser;
    bool m_fatalError;
    bool m_endOfData;
    bool m_treatAllErrorsAsWarnings;

};

class UbuntuClickReviewTaskHandler : public ProjectExplorer::ITaskHandler
{
    Q_OBJECT

    // ITaskHandler interface
public:
    virtual bool canHandle(const ProjectExplorer::Task &task) const;
    virtual void handle(const ProjectExplorer::Task &task);
    virtual QAction *createAction(QObject *parent) const;

private:
    QUrl getUrl (const ProjectExplorer::Task &task) const;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUPACKAGEOUTPUTPARSER_H
