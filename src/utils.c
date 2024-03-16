#include "utils.h"

void report_name_today(char *report_name, int str_size, char *report_prefix, struct tm current_time)
{
    snprintf(
        report_name,
        str_size,
        "%s_%d_%d_%d%s",
        report_prefix,
        current_time.tm_year + 1900,
        current_time.tm_mon + 1,
        current_time.tm_mday,
        REPORT_FILE_EXT);
}

int check_upload(struct tm current_time)
{
    int dir_fd = open(UPLOAD_DIR, O_RDONLY);
    char *report_prefixes[] = {REPORT_PREFIXES};
    int count = 0;

    syslog(LOG_INFO, "{CHECK} Performing reports check in Upload directory...");

    if (dir_fd == -1)
    {
        syslog(LOG_ERR, "{CHECK} Failed to open %s directory: %m", UPLOAD_DIR);
        return -1;
    }

    for (int i = 0; i < NO_OF_DEPTS; i++)
    {
        char report_name[100];
        char report_path[256];

        report_name_today(report_name, sizeof(report_name), report_prefixes[i], current_time);
        snprintf(report_path, sizeof(report_path), "%s%s", UPLOAD_DIR, report_name);

        if (access(report_path, F_OK) == 0)
        {
            count++;
        }
        else
        {
            if (errno == ENOENT)
            {
                syslog(LOG_NOTICE, "{CHECK} %s haven't been uploaded yet", report_name);
            }
            else
            {
                syslog(LOG_ERR, "{CHECK} Failed to check %s: %m", report_name);
            }
        }
    }

    if (count == NO_OF_DEPTS)
    {
        syslog(LOG_INFO, "{CHECK} All reports are uploaded");
    }

    syslog(LOG_INFO, "{CHECK} Reports check complete");
    close(dir_fd);
    return 0;
}