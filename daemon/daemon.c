#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

#define STR_BUFF_SIZE 128

#define WORKING_DIR "/tmp/"

char cfg_path[STR_BUFF_SIZE];

time_t check_period;
char target_path[STR_BUFF_SIZE];

void read_cfg()
{
    char substr1[STR_BUFF_SIZE];
    char substr2[STR_BUFF_SIZE];
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

    strcpy(target_path, substr1);
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
        syslog(LOG_INFO, "Config changed");
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

void execute(const char *path)
{
    struct stat file_stat;
    if (stat(path, &file_stat) < 0)
    {
        syslog(LOG_ERR, "Failed to get stat of %s", path);
        return;
    }

    if (S_ISREG(file_stat.st_mode))
    {
        time_t prev_check_time = time(NULL) - check_period;
        time_t mod_time = file_stat.st_mtime;

        if (mod_time > prev_check_time)
        {
            char time_str[STR_BUFF_SIZE];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&mod_time));

            syslog(LOG_INFO, "%s modified at %s", path, time_str);
        }

        return;
    }

    if (S_ISDIR(file_stat.st_mode))
    {
        DIR *dir = opendir(path);
        if (!dir)
        {
            syslog(LOG_ERR, "Failed to open dir %s", path);
            return;
        }

        struct dirent *entry = readdir(dir);
        while (entry)
        {
            if (entry->d_name[0] == '.')
            {
                entry = readdir(dir);
                continue;
            }

            char new_path[STR_BUFF_SIZE];
            snprintf(new_path, sizeof(new_path), "%s/%s", path, entry->d_name);

            execute(new_path);

            entry = readdir(dir);
        }

        closedir(dir);
        return;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <path to config>\n", argv[0]);
        return EXIT_SUCCESS;
    }

    openlog(argv[0], LOG_PID, LOG_USER);

    strcpy(cfg_path, argv[1]);
    read_cfg();
    launch_daemon();

    syslog(LOG_INFO, "%s launched in %s with %lu check period", argv[0], target_path, check_period);

    while (1)
    {
        sleep(check_period);
        execute(target_path);
    }
}
