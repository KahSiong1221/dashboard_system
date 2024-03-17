#include "init.h"

// Function to create directory if not exists
void mkdir_if_not_exists(char *dir_path, mode_t mode)
{
    errno = 0;
    struct group *grp;
    gid_t gid;
    // root user
    uid_t uid = 0;

    int dir_result = mkdir(dir_path, mode);

    if (dir_result != 0 && errno != EEXIST)
    {
        syslog(LOG_ERR, "[INIT] Failed to create %s: %m", dir_path);
        closelog();
        exit(EXIT_FAILURE);
    }
    else if (dir_result == 0)
    {
        grp = getgrnam(GROUP_NAME);
        if (grp == NULL)
        {
            syslog(LOG_ERR, "[INIT] Failed to get group id: %m");
            closelog();
            exit(EXIT_FAILURE);
        }
        gid = grp->gr_gid;

        if (chown(dir_path, uid, gid) < 0)
        {
            syslog(LOG_ERR, "[INIT] Failed to change ownership of %s: %m", dir_path);
            closelog();
            exit(EXIT_FAILURE);
        }
        if (chmod(dir_path, mode) < 0)
        {
            syslog(LOG_ERR, "[INIT] Failed to change permissions of %s: %m", dir_path);
            closelog();
            exit(EXIT_FAILURE);
        }
        syslog(LOG_INFO, "[INIT] %s is set up successfully", dir_path);
    }
}

// Function to transform main process into a daemon
void daemonize()
{
    pid_t pid;

    syslog(LOG_INFO, "[INIT] Start to create a deamon for dashboard reporting system");

    pid = fork();

    // Exit if fork fails
    if (pid < 0)
    {
        syslog(LOG_ERR, "[INIT] Failed to fork process");
        closelog();
        exit(EXIT_FAILURE);
    }

    // Parent process exits if fork succeeds
    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }

    // Child process becomes session leader, exit if fails
    if (setsid() < 0)
    {
        syslog(LOG_ERR, "[INIT] Failed to create a session");
        closelog();
        exit(EXIT_FAILURE);
    }

    // Ignore specific signals
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    // Second fork to detach completely
    pid = fork();

    // Exit if fork fails
    if (pid < 0)
    {
        syslog(LOG_ERR, "[INIT] Failed to detach during second fork");
        closelog();
        exit(EXIT_FAILURE);
    }

    // Parent process exits if fork succeeds
    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }

    // Change working directory
    chdir("/");

    // Set new file permissions
    umask(0);

    // Close all open file descriptors
    for (int fd = sysconf(_SC_OPEN_MAX); fd >= 0; fd--)
    {
        close(fd);
    }

    // Open log file
    openlog(LOG_IDENT, LOG_PID | LOG_CRON, LOG_DAEMON);
    syslog(LOG_INFO, "[INIT] Daemon started successfully");
}
