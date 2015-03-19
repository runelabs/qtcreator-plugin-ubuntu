#include "ubuntupackageoutputparser.h"
#include <projectexplorer/task.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <QAction>
#include <QDesktopServices>
#include <QRegularExpression>

namespace Ubuntu {
namespace Internal {

const QRegularExpression DEBUG_POLICY_REGEX(QStringLiteral("security_policy_groups_safe_\\S+\\s+\\((\\S+)\\)"));
const QRegularExpression DEBUG_SCOPE_POLICY_REGEX(QStringLiteral("security_policy_groups_scopes"));
const QRegularExpression DEBUG_INAPPROPRIATE_GROUP_TEXT_REGEX(QStringLiteral("found inappropriate policy groups:\\s+(.*)"));
const QRegularExpression DEBUG_UNUSUAL_GROUP_TEXT_REGEX(QStringLiteral("found unusual policy groups:\\s+(.*)"));
const QRegularExpression ARCHITECTURE_ERROR_REGEX(QStringLiteral("lint_(control|manifest)_architecture_valid"));
const QString DEBUG_POLICY_NAME(QStringLiteral("debug"));

UbuntuPackageOutputParser::UbuntuPackageOutputParser() :
    ProjectExplorer::IOutputParser(),
    m_fatalError(false),
    m_endOfData(false)
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
    //commented out due to bug lp:1377094 "Click review errors prevent applications from being deployed to the device"
    //return IOutputParser::hasFatalErrors() || m_fatalError;
    return IOutputParser::hasFatalErrors();
}

/*!
 * \brief UbuntuPackageOutputParser::setEndOfData
 * Tells the parser that the next flush() call can read till end of buffer,
 * because the click-review process has finished
 */
void UbuntuPackageOutputParser::setEndOfData()
{
    m_endOfData = true;
}

void UbuntuPackageOutputParser::onParsedNewTopLevelItem(ClickRunChecksParser::DataItem *item)
{
    emitTasks(item);

    //do not leak the items
    delete item;
}

void UbuntuPackageOutputParser::doFlush()
{
    if(m_endOfData)
        m_subParser.endRecieveData();
    else
        m_subParser.addRecievedData(QString());
}

void UbuntuPackageOutputParser::emitTasks(const ClickRunChecksParser::DataItem *item, int level)
{
    for(int i = 0; i < item->children.size(); i++) {
        emitTasks(item->children[i], level+1);
    }

    if(level == 0)
        return;

    if(item->icon != ClickRunChecksParser::Error && item->icon != ClickRunChecksParser::Warning)
        return;

    bool error = isError(item);
    if(error)
        m_fatalError = true;

    ProjectExplorer::Task task(error ? ProjectExplorer::Task::Error : ProjectExplorer::Task::Warning,
                               QStringLiteral(""), //empty description for now
                               Utils::FileName(),  //we have no file to show
                               -1,                 //line number
                               ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT);

    QString desc = ((QString)QStringLiteral("%1: %2")).arg(item->type).arg(item->text);
    if(item->link.isValid())
        desc.append(QStringLiteral("\n")).append(item->link.toString());

    QRegularExpressionMatch match = DEBUG_POLICY_REGEX.match(item->type);
    if(match.captured(1) == DEBUG_POLICY_NAME)
        desc.append(QStringLiteral("\n")).append(tr("The debug policy group is automatically injected and should only be used for development.\nTo create a package for the store use the publish tab!"));

    task.description = desc;
    addTask(task);
}

bool UbuntuPackageOutputParser::isError(const ClickRunChecksParser::DataItem *item)
{
    bool isError = (item->icon == ClickRunChecksParser::Error);
    if(isError) {
        //add other error item types here if we just want them treated as warnings
        QRegularExpressionMatch match = DEBUG_POLICY_REGEX.match(item->type);
        if(match.hasMatch()) {
            if(match.captured(1) == DEBUG_POLICY_NAME)
                return false;
        }
        match = DEBUG_INAPPROPRIATE_GROUP_TEXT_REGEX.match(item->text);
        if(match.hasMatch()) {
            if(match.captured(1).contains(DEBUG_POLICY_NAME))
                return false;
        }
        match = DEBUG_UNUSUAL_GROUP_TEXT_REGEX.match(item->text);
        if(match.hasMatch()) {
            if(match.captured(1).contains(DEBUG_POLICY_NAME))
                return false;
        }
        match = ARCHITECTURE_ERROR_REGEX.match(item->type);
        if(match.hasMatch()) {
            if(item->text.contains(QStringLiteral("i386")))
                return false;
        }
    }
    return isError;
}

bool UbuntuClickReviewTaskHandler::canHandle(const ProjectExplorer::Task &task) const
{
    return getUrl(task).isValid();
}

void UbuntuClickReviewTaskHandler::handle(const ProjectExplorer::Task &task)
{
    QUrl url = getUrl(task);
    if(url.isValid())
        QDesktopServices::openUrl(url);
}

QAction *UbuntuClickReviewTaskHandler::createAction(QObject *parent) const
{
    QAction *showAction = new QAction(tr("Open URL"), parent);
    showAction->setToolTip(tr("Open the URL."));
    //showAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    return showAction;
}

QUrl UbuntuClickReviewTaskHandler::getUrl(const ProjectExplorer::Task &task) const
{
    QStringList lines = task.description.split(QStringLiteral("\n"));
    if(lines.size() < 2)
        return QUrl();

    QUrl url = QUrl::fromUserInput(lines[1]);
    if(url.isValid() && (url.scheme() == QStringLiteral("http") || url.scheme() == QStringLiteral("https")))
        return url;

    return QUrl();
}

} // namespace Internal
} // namespace Ubuntu
