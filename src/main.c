#include "main.h"

// TODO: create a controller c program
//  to start/stop a daemon, check if it exists first
//  also to manually backup/transfer file/grant access to reporting dir
//
int main(int argc, char *argv[])
{
    time_t now;
    struct tm *current_time;
    pid_t dir_monitor_pid;

    openlog(LOG_IDENT, LOG_PID, LOG_USER);

    mkdir_if_not_exists(STORAGE_DIR, 0770);
    mkdir_if_not_exists(UPLOAD_DIR, 0770);
    mkdir_if_not_exists(BACKUP_DIR, 0750);
    mkdir_if_not_exists(REPORTING_DIR, 0750);

    // Transform into a daemon process
    daemonize();

    // TODO: initialise a signal handler

    // TODO: in the signal handler
    //    perform backup when get signal1
    //    perform transfer when get signal2
    // TODO: a separate program to receive signal from user in CLI

    // Initialise a directory monitor in a child process
    dir_monitor_pid = fork();
    // Exit if fork fails
    if (dir_monitor_pid < 0)
    {
        syslog(LOG_ERR, "[INIT] Failed to fork directory monitoring process");
        closelog();
        exit(EXIT_FAILURE);
    }
    // Child process: Directory monitor
    if (dir_monitor_pid == 0)
    {
        dir_monitor();
    }

    // Parent process: Main daemon
    while (1)
    {
        // get current time
        now = time(NULL);
        current_time = localtime(&now);

        // if its 11.30pm, check if they upload reports
        if (current_time->tm_hour == 23 && current_time->tm_min == 30)
        {
            pid_t child_pid = fork();
            // Exit if fork fails
            if (child_pid < 0)
            {
                syslog(LOG_ERR, "[CHECK] Failed to fork reports checking process");
            }
            // Child process: Check reports
            if (child_pid == 0)
            {
                check_upload(*current_time);
            }
        }
        // if its 1pm, lock upload and report folders
        if (current_time->tm_hour == 18 && current_time->tm_min == 4)
        {
            pid_t child_pid = fork();
            // if fork fails
            if (child_pid < 0)
            {
                syslog(LOG_ERR, "[TRANSFER] Failed to fork lock directories process");
            }
            // Child process: Lock directories
            if (child_pid == 0)
            {
                int lock_status = 0;

                lock_status += lock_dir(UPLOAD_DIR);
                lock_status += lock_dir(REPORTING_DIR);

                if (lock_status < 0)
                {
                    sleep(30);
                    lock_status = 0;
                    lock_status += lock_dir(UPLOAD_DIR);
                    lock_status += lock_dir(REPORTING_DIR);

                    if (lock_status < 0)
                    {
                        syslog(LOG_ERR, "[TRANSFER] Failed to lock down directories twice, backup and transfer process terminated");
                        exit(EXIT_FAILURE);
                    }
                }

                syslog(LOG_INFO, "[TRANSFER] Locked %s and %s for reports transfer and backup", UPLOAD_DIR, REPORTING_DIR);

                pid_t cchild_pid;

                cchild_pid = fork();

                if (cchild_pid < 0)
                {
                    syslog(LOG_ERR, "[TRANSFER] Failed to fork backup and transfer process");
                }
                // Child process: Backup & transfer reports
                if (cchild_pid == 0)
                {
                    time_t yesterday = now - 24 * 60 * 60;
                    struct tm *timeinfo = localtime(&yesterday);

                    auto_backup_transfer_reports(*timeinfo);
                }

                // Parent process: Wait & Unlock directories
                waitpid(cchild_pid, NULL, 0);

                int unlock_status = 0;

                unlock_status += unlock_dir(UPLOAD_DIR, 0770);
                unlock_status += unlock_dir(REPORTING_DIR, 0750);

                if (unlock_status < 0)
                {
                    sleep(30);
                    unlock_status = 0;
                    unlock_status += unlock_dir(UPLOAD_DIR, 0770);
                    unlock_status += unlock_dir(REPORTING_DIR, 0750);

                    if (unlock_status < 0)
                    {
                        syslog(LOG_ERR, "[TRANSFER] Failed to unlock directories twice, please unlock them manually");
                        exit(EXIT_FAILURE);
                    }
                }

                syslog(LOG_INFO, "[TRANSFER] Unlocked %s and %s", UPLOAD_DIR, REPORTING_DIR);

                exit(EXIT_SUCCESS);
            }
        }
        sleep(60);
    }

    syslog(LOG_INFO, "Daemon terminated");
    closelog();

    return EXIT_SUCCESS;
}
