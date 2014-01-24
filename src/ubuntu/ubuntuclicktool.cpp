#include "ubuntuclicktool.h"

#include <QRegExp>
#include <QDir>
#include <QDebug>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QFont>
#include <QFormLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QMessageBox>

#include <coreplugin/icore.h>
#include <utils/qtcprocess.h>
#include <utils/environment.h>
#include <utils/consoleprocess.h>
#include <texteditor/fontsettings.h>

namespace Ubuntu {
namespace Internal {

/**
 * @brief UbuntuClickTool::UbuntuClickTool
 * Implements functionality needed for executing the click
 * tool
 */
UbuntuClickTool::UbuntuClickTool()
{
}

/**
 * @brief UbuntuClickTool::parametersForCreateChroot
 * Initializes a ProjectExplorer::ProcessParameters object with command and arguments
 * to create a new chroot
 */
void UbuntuClickTool::parametersForCreateChroot(const QString &arch, const QString &series, ProjectExplorer::ProcessParameters *params)
{
    params->setCommand(QLatin1String("pkexec"));

    QStringList clickArgs;
    clickArgs << QLatin1String("click")
              << QLatin1String("chroot")
              << QLatin1String("-a")
              << arch
              << QLatin1String("-s")
              << series
              << QLatin1String("create");

    QStringList sudoArgs;
    sudoArgs << QLatin1String("sh")
             << QLatin1String("-c")
             << Utils::QtcProcess::joinArgs(clickArgs);

    params->setEnvironment(Utils::Environment::systemEnvironment());
    params->setArguments(Utils::QtcProcess::joinArgs(sudoArgs));
}

/**
 * @brief UbuntuClickTool::parametersForMaintainChroot
 * Initializes params with the arguments for maintaining the chroot
 * @note does not call ProjectExplorer::ProcessParameters::resolveAll()
 */
void UbuntuClickTool::parametersForMaintainChroot(const UbuntuClickTool::MaintainMode &mode, const Target &target, ProjectExplorer::ProcessParameters *params)
{
    params->setCommand(QLatin1String("click"));

    QString strMode;
    switch (mode) {
    case Upgrade:
        strMode = QLatin1String("upgrade");
        break;
    case Delete:
        strMode = QLatin1String("destroy");
        break;
    }

    QStringList arguments;
    arguments << QLatin1String("chroot")
              << QLatin1String("-a")
              << target.architecture
              << QLatin1String("-f")
              << target.framework
              << strMode;

    params->setEnvironment(Utils::Environment::systemEnvironment());
    params->setArguments(Utils::QtcProcess::joinArgs(arguments));
}

/**
 * @brief UbuntuClickTool::parametersForCmake
 * Fills ProcessParameters to run cmake inside the Target
 * @note does not call ProjectExplorer::ProcessParameters::resolveAll()
 */
void UbuntuClickTool::parametersForCmake(const Target &target, const QString &buildDir
                                         ,const QString &relPathToSource, ProjectExplorer::ProcessParameters *params)
{
    QStringList arguments;
    arguments << QLatin1String("chroot")
              << QLatin1String("-a")
              << target.architecture
              << QLatin1String("-f")
              << target.framework
              << QLatin1String("run")
              << QLatin1String("cmake")
              << QLatin1String("-DCMAKE_TOOLCHAIN_FILE=/etc/dpkg-cross/cmake/CMakeCross.txt")
              << relPathToSource;

    params->setWorkingDirectory(buildDir);
    params->setCommand(QLatin1String("click"));
    params->setArguments(Utils::QtcProcess::joinArgs(arguments));
}

/**
 * @brief UbuntuClickTool::parametersForMake
 * Fills ProcessParameters to run make inside the Target
 * @note does not call ProjectExplorer::ProcessParameters::resolveAll()
 */
void UbuntuClickTool::parametersForMake(const UbuntuClickTool::Target &target, const QString &buildDir
                                        , ProjectExplorer::ProcessParameters *params)
{
    QStringList arguments;
    arguments << QLatin1String("chroot")
              << QLatin1String("-a")
              << target.architecture
              << QLatin1String("-f")
              << target.framework
              << QLatin1String("run")
              << QLatin1String("make");

    params->setWorkingDirectory(buildDir);
    params->setCommand(QLatin1String("click"));
    params->setArguments(Utils::QtcProcess::joinArgs(arguments));
}

/**
 * @brief UbuntuClickTool::openChrootTerminal
 * Opens a new terminal logged into the chroot specified by \a target
 * The terminal emulator used is specified in the Creator environment option page
 */
void UbuntuClickTool::openChrootTerminal(const UbuntuClickTool::Target &target)
{
    QStringList args = Utils::QtcProcess::splitArgs(Utils::ConsoleProcess::terminalEmulator(Core::ICore::settings()));
    QString     term = args.takeFirst();

    qDebug()<<"Going to use terminal emulator: "<<term;

    args << QString(QLatin1String("click chroot -a %0 -f %1 maint /bin/bash")).arg(target.architecture).arg(target.framework);
    if(!QProcess::startDetached(term,args,QDir::homePath())) {
        qDebug()<<"Error when starting terminal";
    }
}

/**
 * @brief UbuntuClickTool::listAvailableTargets
 * @return all currently existing chroot targets in the system
 */
QList<UbuntuClickTool::Target> UbuntuClickTool::listAvailableTargets()
{
    QList<Target> items;
    QDir chrootDir(QLatin1String("/var/lib/schroot/chroots"));

    //if the dir does not exist there are no available chroots
    if(!chrootDir.exists())
        return items;

    QStringList availableChroots = chrootDir.entryList(QDir::Dirs);

    QRegExp clickFilter(QLatin1String("^click-(.*)-([A-Za-z0-9]+)$"));

    //iterate over all chroots and check if they are click chroots
    foreach (const QString &chroot, availableChroots) {
        if(!clickFilter.exactMatch(chroot)) {
            qDebug()<<"Skipping: "<<chroot<<" Matched: "<<clickFilter.matchedLength();
            continue;
        }

        //we need at least 3 captures
        if ( clickFilter.captureCount() < 2 ) {
            qDebug()<<"Skipping: "<<chroot<<" Not enough matches: "<<clickFilter.capturedTexts();
            continue;
        }

        Target t;
        t.framework    = clickFilter.cap(1);
        t.architecture = clickFilter.cap(2);
        items.append(t);
    }

    return items;
}

UbuntuClickDialog::UbuntuClickDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Run Click"));

    QFormLayout* layout = new QFormLayout(this);
    this->setLayout(layout);

    m_output = new QPlainTextEdit();

    //setup the output window in the way the cmake plugin does
    //typewriter is just better for console output
    m_output->setMinimumHeight(15);
    QFont f(TextEditor::FontSettings::defaultFixedFontFamily());
    f.setStyleHint(QFont::TypeWriter);
    m_output->setFont(f);

    QSizePolicy pl = m_output->sizePolicy();
    pl.setVerticalStretch(1);
    m_output->setSizePolicy(pl);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    m_exitStatus = new QLabel();
    m_exitStatus->setVisible(false);

    layout->addRow(new QLabel(tr("Run Click")));
    layout->addRow(m_exitStatus);
    layout->addRow(m_output);
    layout->addRow(m_buttonBox);

    m_process = new Utils::QtcProcess(this);
    connect(m_process,SIGNAL(readyReadStandardOutput()),this,SLOT(on_clickReadyReadStandardOutput()));
    connect(m_process,SIGNAL(readyReadStandardError()),this,SLOT(on_clickReadyReadStandardError()));
    connect(m_process,SIGNAL(finished(int)),this,SLOT(on_clickFinished(int)));

    setMinimumSize(640,480);
}

UbuntuClickDialog::~UbuntuClickDialog()
{

}

void UbuntuClickDialog::setParameters(ProjectExplorer::ProcessParameters *params)
{
    params->resolveAll();
    m_process->setCommand(params->command(),params->arguments());
    m_process->setEnvironment(params->environment());
    m_process->setWorkingDirectory(params->workingDirectory());
}

void UbuntuClickDialog::runClick( )
{
    //change the button to cancel
    m_buttonBox->clear();
    m_buttonBox->addButton(QDialogButtonBox::Cancel);

    m_process->start();
}

void UbuntuClickDialog::runClickModal(ProjectExplorer::ProcessParameters *params)
{
    UbuntuClickDialog dlg;
    dlg.setParameters(params);
    QMetaObject::invokeMethod(&dlg,"runClick",Qt::QueuedConnection);
    dlg.exec();
}

void UbuntuClickDialog::createClickChrootModal()
{
    const char* supportedArchs[]  = {"armhf","i386","amd64","\0"};
    const char* supportedSeries[] = {"saucy","trusty","\0"};

    QDialog dlg;
    QFormLayout* layout = new QFormLayout(&dlg);
    dlg.setLayout(layout);

    //add supported architectures
    QComboBox* cbArch = new QComboBox(&dlg);
    for(int i = 0; supportedArchs[i][0] != '\0' ;i++) {
        cbArch->addItem(QLatin1String(supportedArchs[i]));
    }

    //add supported series
    QComboBox* cbSeries = new QComboBox(&dlg);
    for(int i = 0; supportedSeries[i][0] != '\0' ;i++) {
        cbSeries->addItem(QLatin1String(supportedSeries[i]));
    }

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), &dlg, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), &dlg, SLOT(reject()));

    layout->addRow(tr("Architecture"),cbArch);
    layout->addRow(tr("Series"),cbSeries);
    layout->addRow(buttonBox);

    if(dlg.exec() == QDialog::Accepted) {
        ProjectExplorer::ProcessParameters params;
        qDebug()<<"Creating click chroot arch: "<<cbArch->currentText()<<" series: "<<cbSeries->currentText();
        UbuntuClickTool::parametersForCreateChroot(cbArch->currentText(),cbSeries->currentText(),&params);
        runClickModal(&params);
    }
}

void UbuntuClickDialog::maintainClickModal(const UbuntuClickTool::Target &target, const UbuntuClickTool::MaintainMode &mode)
{
    if(mode == UbuntuClickTool::Delete) {
        QString title = tr("Delete click chroot");
        QString text  = tr("Are you sure you want to delete this chroot?");
        if( QMessageBox::question(0,title,text) != QMessageBox::Yes )
            return;
    }

    ProjectExplorer::ProcessParameters params;
    UbuntuClickTool::parametersForMaintainChroot(mode,target,&params);
    runClickModal(&params);
}

void UbuntuClickDialog::done(int code)
{
    if(code == QDialog::Rejected) {
        if(m_process->state() != QProcess::NotRunning) {
            //ask the user if he really wants to do that
            QString title = tr("Stop click tool");
            QString text  = tr("Are you sure you want to stop click? This could break your chroot!");
            if( QMessageBox::question(0,title,text)!= QMessageBox::Yes )
                return;

            m_process->terminate();
            m_process->waitForFinished(100);
            m_process->kill();

            m_exitStatus->setText(tr("Waiting for click to stop"));
            m_exitStatus->setVisible(true);
            return;
        }
    }
    QDialog::done(code);
}

void UbuntuClickDialog::on_clickFinished(int exitCode)
{
    //set the button to close again
    m_buttonBox->clear();
    m_buttonBox->addButton(QDialogButtonBox::Close);

    if (exitCode != 0) {
        on_clickReadyReadStandardError(tr("---Click exited with errors, please check the output---"));
    } else {
        on_clickReadyReadStandardOutput(tr("---Click exited with no errors---"));
    }
}

void UbuntuClickDialog::on_clickReadyReadStandardOutput(const QString txt)
{
    QTextCursor cursor(m_output->document());
    cursor.movePosition(QTextCursor::End);
    QTextCharFormat tf;

    QFont font = m_output->font();
    tf.setFont(font);
    tf.setForeground(m_output->palette().color(QPalette::Text));

    if(txt.isEmpty())
        cursor.insertText(QString::fromLocal8Bit(m_process->readAllStandardOutput()), tf);
    else
        cursor.insertText(txt, tf);
}

void UbuntuClickDialog::on_clickReadyReadStandardError(const QString txt)
{
    QTextCursor cursor(m_output->document());
    QTextCharFormat tf;

    QFont font = m_output->font();
    QFont boldFont = font;
    boldFont.setBold(true);
    tf.setFont(boldFont);
    tf.setForeground(QColor(Qt::red));

    if(txt.isEmpty())
        cursor.insertText(QString::fromLocal8Bit(m_process->readAllStandardError()), tf);
    else
        cursor.insertText(txt, tf);
}

} // namespace Internal
} // namespace Ubuntu

