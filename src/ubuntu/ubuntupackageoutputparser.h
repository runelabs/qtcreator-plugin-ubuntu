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
    virtual void stdOutput(const QString &line) override;
    virtual void stdError(const QString &line) override;
    virtual bool hasFatalErrors() const override;

    void setTreatAllErrorsAsWarnings ( const bool set );

public slots:
    void setEndOfData ();

private slots:
    void onParsedNewTopLevelItem (ClickRunChecksParser::DataItem* item);

private:
    virtual void doFlush() override;
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
    virtual bool canHandle(const ProjectExplorer::Task &task) const override;
    virtual void handle(const ProjectExplorer::Task &task) override;
    virtual QAction *createAction(QObject *parent) const override;

private:
    QUrl getUrl (const ProjectExplorer::Task &task) const;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUPACKAGEOUTPUTPARSER_H
