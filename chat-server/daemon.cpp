#include "daemon.h"

void signal_handler(int signal)
{
    switch (signal)
    {
    case SIGTERM:
        syslog(LOG_INFO, "SIGTERM accepted");
        exit(EXIT_SUCCESS);
        break;
    }
}

void daemonize()
{
    pid_t pid = fork();

    if (pid < 0)
    {
        syslog(LOG_ERR, "Fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }

    pid_t sid = setsid();

    if (sid < 0)
    {
        syslog(LOG_ERR, "SID set failed");
        exit(EXIT_FAILURE);
    }

    umask(0);

    if (chdir(WORKING_DIR) < 0)
    {
        syslog(LOG_ERR, "Working dir change failed");
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    signal(SIGTERM, signal_handler);
}
