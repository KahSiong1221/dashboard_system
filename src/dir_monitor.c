#include "dir_monitor.h"

void dir_monitor()
{
    errno = 0;
    int inotify_fd, wd, i;
    char buffer[BUFFER_LEN];
    ssize_t bytes_read;
    struct inotify_event *event;

    inotify_fd = inotify_init();

    if (inotify_fd < 0)
    {
        syslog(LOG_ERR, "[MONITOR] Failed to initialise inotify: %m");
        closelog();
        exit(EXIT_FAILURE);
    }

    wd = inotify_add_watch(inotify_fd, UPLOAD_DIR, IN_CREATE | IN_DELETE | IN_MODIFY);

    if (wd < 0)
    {
        syslog(LOG_ERR, "[MONITOR] Failed to add watch on directory: %m");
        closelog();
        close(inotify_fd);
        exit(EXIT_FAILURE);
    }

    syslog(LOG_INFO, "[MONITOR] Monitoring directory %s", UPLOAD_DIR);

    while (1)
    {
        i = 0;
        bytes_read = read(inotify_fd, buffer, BUFFER_LEN);

        if (bytes_read < 0)
        {
            syslog(LOG_WARNING, "[MONITOR] Failed to read inotify event: $m");
            continue;
        }

        // Loop through all events in the buffer
        while (i < bytes_read)
        {
            event = (struct inotify_event *)&buffer[i];
            if (event->len)
            {
                const char *event_str;

                if (event->mask & IN_CREATE)
                {
                    event_str = "created";
                }
                else if (event->mask & IN_DELETE)
                {
                    event_str = "deleted";
                }
                else if (event->mask & IN_MODIFY)
                {
                    event_str = "modified";
                }
                else
                {
                    event_str = "performed unknown action on";
                }

                syslog(LOG_INFO, "[MONITOR] %s %s", event_str, event->name);
            }
            i += EVENT_SIZE + event->len;
        }
    }
    inotify_rm_watch(inotify_fd, wd);
    close(inotify_fd);
}
