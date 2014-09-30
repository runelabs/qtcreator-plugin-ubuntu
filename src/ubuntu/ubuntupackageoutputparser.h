#ifndef UBUNTU_INTERNAL_UBUNTUPACKAGEOUTPUTPARSER_H
#define UBUNTU_INTERNAL_UBUNTUPACKAGEOUTPUTPARSER_H

#include <projectexplorer/ioutputparser.h>
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

private slots:
    void onParsedNewTopLevelItem (ClickRunChecksParser::DataItem* item);

private:
    virtual void doFlush();
    void emitTasks (const ClickRunChecksParser::DataItem *item);
    ClickRunChecksParser m_subParser;

};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUPACKAGEOUTPUTPARSER_H
