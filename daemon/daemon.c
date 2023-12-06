#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define STR_BUF_SIZE 64

#define DAEMON_NAME "dirlogger"
#define WORKING_DIR "/tmp/"

const char *cfg_path;

unsigned long check_period;
const char *target_dir;

void read_cfg()
{
    char substr1[STR_BUF_SIZE];
    char substr2[STR_BUF_SIZE];
    FILE *file = fopen(cfg_path, "r");

    if (!file)
    {
        syslog(LOG_ERR, "Error opening config");
        exit(EXIT_FAILURE);
    }

    if (fscanf(file, "%s %s", substr1, substr2) != 2)
    {
        syslog(LOG_ERR, "Incorrect config content");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    DIR *dir = opendir(substr1);
    if (!dir)
    {
        syslog(LOG_ERR, "Error opening target directory");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    closedir(dir);

    unsigned long period = strtoul(substr2, NULL, 10);
    if (period <= 0)
    {
        syslog(LOG_ERR, "Incorrect check period");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    strcpy(target_dir, substr1);
    check_period = period;

    fclose(file);
}

void signal_handler(int signal)
{
    switch (signal)
    {
    case SIGHUP:
        syslog(LOG_INFO, "SIGHUP accepted");
        read_cfg();
        syslog(LOG_INFO, "Target dir changed");
        break;

    case SIGTERM:
        syslog(LOG_INFO, "SIGTERM accepted");
        exit(EXIT_SUCCESS);
        break;

    default:
        syslog(LOG_ERR, "Unexpected signal");
    }
}

void launch_daemon()
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

    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);
}

void execute()
{
    // TODO: Implement!
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <path to config>\n", argv[0]);
        return EXIT_FAILURE;
    }

    openlog(DAEMON_NAME, LOG_PID, LOG_USER);

    strcpy(cfg_path, argv[1]);
    read_cfg();
    launch_daemon();

    syslog(LOG_INFO, "%s launched in %s with %lu check period", DAEMON_NAME, target_dir, check_period);

    while (1)
    {
        sleep(check_period);
        execute();
    }
}
