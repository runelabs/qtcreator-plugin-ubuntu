/**
  *
  * based on code taken from http://www.enderunix.org/docs/eng/daemon.php
  */

#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <pwd.h>
#include <stdlib.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

#include <QCoreApplication>
#include <QDir>
#include <QDBusConnection>
#include <QDebug>
#include <QDBusError>
#include <QStandardPaths>

#include "chrootagent.h"
#include "clickchrootagent_interface.h"

int  cliMode (int argc, char *argv[]);
int  serviceMode (int argc, char *argv[]);
void log_message (const char *message, int logType = LOG_INFO);
void signal_handler(int sig);
void daemonize();
void useage ();

void syslogMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    bool do_abort = false;
    openlog ("click-chroot-agent", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        syslog (LOG_DEBUG,  "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        syslog (LOG_WARNING,"Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        syslog (LOG_CRIT,   "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        syslog (LOG_EMERG,  "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        do_abort=true;
    }

    closelog();

    if(do_abort) abort();
}

void log_message (const char *message, int logType)
{
    openlog ("click-chroot-agent", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    syslog (logType,  message,0);
    closelog();
}

void signal_handler(int sig)
{
    switch(sig) {
    case SIGHUP:
        log_message("hangup signal catched");
        ChrootAgent::instance()->hangup();
        break;
    case SIGTERM:
        log_message("terminate signal catched");
        ChrootAgent::instance()->shutdown();
        break;
    }
}

void daemonize()
{
    if(getppid()==1) return; /* already a daemon */

    int i=fork();
    if (i<0) {
        log_message("Could not fork.",LOG_EMERG);
        exit(1); /* fork error */
    }
    if (i>0) {
        _exit(0); /* parent exits */
    }

    /* child (daemon) continues */
    setsid(); /* obtain a new process group */
    for (i=getdtablesize();i>=0;--i)
        close(i); /* close all descriptors */

    /* handle standart I/O */
    i=open("/dev/null",O_RDWR);
    if(dup(i) < 0)
        log_message("Could not redirect std filedescriptor",LOG_ERR);
    if(dup(i) < 0)
        log_message("Could not redirect std filedescriptor",LOG_ERR);

    umask(027); /* set newly created file permissions */

    QString lockFile = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation)+
            QDir::separator()+
            QStringLiteral("click-chroot-agent.pid");

    int lfp=open(qPrintable(lockFile),O_RDWR|O_CREAT,0640);
    if (lfp<0) {
        log_message("Can not open lockfile",LOG_ERR);
        exit(1); /* can not open */
    }

    if (lockf(lfp,F_TLOCK,0)<0) {
        log_message("Can not lock lockfile, click-chroot-agent is probably already running.");
        exit(2); /* can not lock, server already running */
    }

    /* first instance continues */
    QByteArray arr = QByteArray::number(getpid());
    if(write(lfp,arr.constData(),strlen(arr.constData())) < 0) /* record pid to lockfile */
        log_message("Could not write pid to the lockfile");

    signal(SIGCHLD,SIG_IGN); /* ignore child */
    signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
    signal(SIGTTOU,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);
}

void useage ()
{
    puts("Usage: click-chroot-agent [OPTION]");
    puts("-s, --stop    Stops the currently running instance of the click-chroot-agent");
    puts("-r, --reload  Recreates sessions for all exising chroots");
    puts("-i, --info    Shows currently mounted sessions");
    puts("-h, --help    Shows this text");
}

/*!
 * \brief cliMode
 * Runs the command in CLI mode to allow a more convenient use
 */
int cliMode (int argc, char *argv[])
{
    if(argc > 2) {
        puts("Too many arguments.");

        useage();
        return 0;
    }

    QCoreApplication a(argc, argv);
    Q_UNUSED(a);

    static struct option long_options[] = {
        {"stop",    no_argument,       0, 's'},
        {"reload",  no_argument,       0, 'r'},
        {"info",    no_argument,       0, 'i'},
        {"help",    no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    ComUbuntuSdkClickChrootAgentInterface clickAgent(QStringLiteral("com.ubuntu.sdk.ClickChrootAgent"),
                                                     QStringLiteral("/com/ubuntu/sdk/ClickChrootAgent"),
                                                     QDBusConnection::sessionBus());
    if(!clickAgent.isValid()) {
        puts("Could not connect to click-chroot-agent service");
        return 1;
    }

    bool cont = true;
    while(cont) {
        int option_index = 0;
        int c = getopt_long (argc, argv, "srih",long_options, &option_index);

        if (c == -1)
            break;

        switch(c) {
            case 's': {
                clickAgent.shutdown();
                cont = false;
                break;
            }
            case 'r': {
                QDBusPendingReply<void> ret = clickAgent.hangup();
                if(ret.isError())
                    puts(qPrintable(ret.error().message()));
                else
                    ret.waitForFinished();
                cont = false;
                break;
            }
            case 'i': {
                QDBusPendingReply<QStringList> ret = clickAgent.sessionInfo();
                if(ret.isError()) {
                    puts(qPrintable(ret.error().message()));
                } else {
                    ret.waitForFinished();
                    foreach(const QString &desc, ret.value()) {
                        puts(qPrintable(desc));
                    }
                }
                cont = false;
                break;
            }
            case 'h':
            case '?': {
                cont = false;
                useage();
                break;
            }
        }
    }
    return 0;
}

/*!
 * \brief serviceMode
 * starts the click-chroot-agents service and returns only if its shut down
 */
int serviceMode (int argc, char *argv[])
{
    qInstallMessageHandler(syslogMessageHandler);
    QCoreApplication a(argc, argv);

    //the control signals
    signal(SIGHUP,signal_handler); /* catch hangup signal */
    signal(SIGTERM,signal_handler); /* catch term signal */

    ChrootAgent agent;

    QDBusConnection sessionBus = QDBusConnection::sessionBus();

    //when the session bus goes down we want to exit
    if(!sessionBus.connect(QString(),QString(),QStringLiteral("org.freedesktop.DBus.Local"),QStringLiteral("Disconnected"), &agent, SLOT(shutdown()))) {
        log_message("Could not connect to DBUS session bus",LOG_EMERG);
        return 1;
    }

    if(!sessionBus.registerObject(QStringLiteral("/com/ubuntu/sdk/ClickChrootAgent"),&agent)) {
        log_message("Could not register DBUS interface",LOG_EMERG);
        qDebug()<<sessionBus.lastError();
        return 1;
    }

    if(!sessionBus.registerService(QStringLiteral("com.ubuntu.sdk.ClickChrootAgent"))) {
        log_message("Could not register DBUS service",LOG_EMERG);
        qDebug()<<sessionBus.lastError();
        return 1;
    }

    return a.exec();
}

int main(int argc, char *argv[])
{
    if(argc > 1)
        return cliMode(argc,argv);

    daemonize();
    return serviceMode(argc,argv);
}
