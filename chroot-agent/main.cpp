/**
  *
  * fork code taken from http://www.enderunix.org/docs/eng/daemon.php
  */

#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

#include <QCoreApplication>
#include <QDir>
#include <QDBusConnection>
#include <QDebug>
#include <QDBusError>

#include "chrootagent.h"

#define RUNNING_DIR	"/tmp"
#define LOCK_FILE	"/home/zbenjamin/.cache/ubuntu-sdk-chroot-agent.pid"

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

void log_message (const char *message, int logType = LOG_INFO)
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
        exit(0); /* parent exits */
    }

    /* child (daemon) continues */
    setsid(); /* obtain a new process group */
    for (i=getdtablesize();i>=0;--i)
        close(i); /* close all descriptors */

    /* handle standart I/O */
    i=open("/dev/null",O_RDWR);
    dup(i);
    dup(i);

    umask(027); /* set newly created file permissions */

    int lfp=open(LOCK_FILE,O_RDWR|O_CREAT,0640);
    if (lfp<0) {
        log_message("Can not open lockfile");
        exit(1); /* can not open */
    }

    if (lockf(lfp,F_TLOCK,0)<0) {
        log_message("Can not lock lockfile");
        exit(2); /* can not lock, server already running */
    }

    /* first instance continues */
    char str[10];
    sprintf(str,"%d\n",getpid());
    write(lfp,str,strlen(str)); /* record pid to lockfile */
    signal(SIGCHLD,SIG_IGN); /* ignore child */
    signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
    signal(SIGTTOU,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);
}

int main(int argc, char *argv[])
{
    daemonize();

    //the control signals
    signal(SIGHUP,signal_handler); /* catch hangup signal */
    signal(SIGTERM,signal_handler); /* catch term signal */

    qInstallMessageHandler(syslogMessageHandler);
    QCoreApplication a(argc, argv);

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
