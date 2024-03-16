#include "main.h"

// TODO: create a controller c program
//  to start/stop a daemon, check if it exists first
//  also to manually backup/transfer file/grant access to reporting dir
//
int main(int argc, char *argv[])
{
    time_t now;
    struct tm *current_time;
    bool is_checked = false;
    bool is_transferred = false;

    openlog(LOG_IDENT, LOG_PID, LOG_USER);

    mkdir_if_not_exists(STORAGE_DIR, 0770);
    mkdir_if_not_exists(UPLOAD_DIR, 0660);
    mkdir_if_not_exists(BACKUP_DIR, 0660);
    mkdir_if_not_exists(REPORTING_DIR, 0640);

    // Transform into a daemon process
    daemon_init();

    // TODO: initialise a directory monitor
    // TODO: initialise a signal handler

    // TODO: in the signal handler
    //    perform backup when get signal1
    //    perform transfer when get signal2
    // TODO: a separate program to receive signal from user in CLI

    // Main loop: stay for 2 minutes
    while (1)
    {
        // get current time
        now = time(NULL);
        current_time = localtime(&now);

        // if its 11.30pm, check if they upload reports
        if (current_time->tm_hour == 23 && current_time->tm_min >= 30 && is_checked == false)
        {
            if (check_upload(*current_time) == 0)
            {
                is_checked = true;
            }
        }
        // if its 1pm, lock upload and report folders
        if (current_time->tm_hour == 1 && is_checked == true && is_transferred == false)
        {
            // TODO: lock folders
            // TODO: transfer reports
            is_checked = false;
            is_transferred = true;
        }
        // TODO: backup reporting folder as well

        sleep(60);
        break;
    }

    syslog(LOG_INFO, "Daemon terminated");
    closelog();

    return EXIT_SUCCESS;
}
