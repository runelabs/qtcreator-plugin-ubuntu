#include "ubuntupackageoutputparser.h"
#include <projectexplorer/task.h>
#include <projectexplorer/projectexplorerconstants.h>

namespace Ubuntu {
namespace Internal {

UbuntuPackageOutputParser::UbuntuPackageOutputParser() :
    ProjectExplorer::IOutputParser()
{
    m_subParser.beginRecieveData();
    connect(&m_subParser,&ClickRunChecksParser::parsedNewTopLevelItem,this,&UbuntuPackageOutputParser::onParsedNewTopLevelItem);
}

void UbuntuPackageOutputParser::stdOutput(const QString &line)
{
    IOutputParser::stdOutput(line);
    m_subParser.addRecievedData(line);
}

void UbuntuPackageOutputParser::stdError(const QString &line)
{
    IOutputParser::stdError(line);
    m_subParser.addRecievedData(line);
}

bool UbuntuPackageOutputParser::hasFatalErrors() const
{
    return IOutputParser::hasFatalErrors();
}

void UbuntuPackageOutputParser::onParsedNewTopLevelItem(ClickRunChecksParser::DataItem *item)
{
    emitTasks(item);
    delete item;
}
void UbuntuPackageOutputParser::doFlush()
{
    m_subParser.endRecieveData();
}

void UbuntuPackageOutputParser::emitTasks(const ClickRunChecksParser::DataItem *item)
{
    for(int i = 0; i < item->children.size(); i++) {
        emitTasks(item->children[i]);
    }

    if(item->icon == ClickRunChecksParser::Error || item->icon == ClickRunChecksParser::Warning) {
        ProjectExplorer::Task task;
        task.type = ProjectExplorer::Task::Warning;
        task.category = ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT;
        task.description = QString(QStringLiteral("%1: %2 \n %3"))
                .arg(item->type)
                .arg(item->text)
                .arg(item->link.toString());
        addTask(task);
    }
}

} // namespace Internal
} // namespace Ubuntu
