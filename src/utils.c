#include "utils.h"

void report_name_today(char *report_name, int str_size, const char *report_prefix, struct tm current_time)
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

void check_upload(struct tm current_time)
{
    int dir_fd = open(UPLOAD_DIR, O_RDONLY);
    const char *report_prefixes[] = {REPORT_PREFIXES};
    int count = 0;

    syslog(LOG_INFO, "[CHECK] Performing reports check in Upload directory...");

    if (dir_fd == -1)
    {
        syslog(LOG_ERR, "[CHECK] Failed to open %s directory: %m", UPLOAD_DIR);
        exit(EXIT_FAILURE);
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
                syslog(LOG_NOTICE, "[CHECK] %s haven't been uploaded yet", report_name);
            }
            else
            {
                syslog(LOG_ERR, "[CHECK] Failed to check %s: %m", report_name);
            }
        }
    }

    if (count == NO_OF_DEPTS)
    {
        syslog(LOG_INFO, "[CHECK] All reports are uploaded");
    }

    syslog(LOG_INFO, "[CHECK] Reports check complete");
    close(dir_fd);
    exit(EXIT_SUCCESS);
}

int lock_dir(char *dir_path)
{
    if (chmod(dir_path, 0550) < 0)
    {
        syslog(LOG_ERR, "[TRANSFER] Failed to lock %s: %m", dir_path);
        return -1;
    }
    return 0;
}

int unlock_dir(char *dir_path, mode_t mode)
{
    if (chmod(dir_path, mode) < 0)
    {
        syslog(LOG_ERR, "[TRANSFER] Failed to unlock %s: %m", dir_path);
        return -1;
    }
    return 0;
}

int is_report(const char *filename, char report_names[NO_OF_DEPTS][100])
{
    for (int i = 0; i < NO_OF_DEPTS; ++i)
    {
        if (strcmp(filename, report_names[i]) == 0)
        {
            return i;
        }
    }
    return -1;
}

int is_allowed_prefix(const char *filename, const char *report_prefixes[])
{
    for (int i = 0; i < NO_OF_DEPTS; ++i)
    {
        if (strncmp(filename, report_prefixes[i], strlen(report_prefixes[i])) == 0)
        {
            return 1;
        }
    }
    return 0;
}

int is_xml_file(const char *filename)
{
    int len = strlen(filename);
    return len >= 4 && strcmp(filename + len - 4, REPORT_FILE_EXT) == 0;
}

void copy_report(const char *source_path, const char *target_path)
{
    execl("/bin/cp", "cp", source_path, target_path, NULL);
    // if execl failed
    syslog(LOG_ERR, "[Transfer] Failed to copy %s to %s: %m", source_path, target_path);
    exit(EXIT_FAILURE);
}

void remove_report(const char *source_path)
{
    execl("/bin/rm", "rm", source_path, NULL);
    // if execl failed
    syslog(LOG_ERR, "[Transfer] Failed to remove %s: %m", source_path);
    exit(EXIT_FAILURE);
}

void auto_backup_transfer_reports(struct tm timeinfo)
{
    const char *report_prefixes[] = {REPORT_PREFIXES};
    char report_names[NO_OF_DEPTS][100];
    int report_status[NO_OF_DEPTS] = {0, 0, 0, 0};
    int job_status = 0;

    for (int i = 0; i < NO_OF_DEPTS; i++)
    {
        report_name_today(report_names[i], sizeof(report_names[i]), report_prefixes[i], timeinfo);
    }

    DIR *dir = opendir(UPLOAD_DIR);
    struct dirent *entry;

    if (dir == NULL)
    {
        syslog(LOG_ERR, "[TRANSFER] Failed to open %s: %m", UPLOAD_DIR);
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        const char *filename = entry->d_name;
        int report_index = is_report(filename, report_names);

        if (report_index < 0)
        {
            syslog(LOG_WARNING, "[TRANSFER] Unknown file %s in %s: not follow file naming convention or report is out of date", filename, UPLOAD_DIR);
            continue;
        }

        char source_path[256];

        snprintf(source_path, sizeof(source_path), "%s/%s", UPLOAD_DIR, filename);

        pid_t transfer_pid = fork();

        if (transfer_pid < 0)
        {
            syslog(LOG_ERR, "[TRANSFER] Failed to fork transfer process for %s", filename);
            continue;
        }
        // Child process: Transfer reports
        if (transfer_pid == 0)
        {
            copy_report(source_path, REPORTING_DIR);
        }

        pid_t backup_pid = fork();

        if (backup_pid < 0)
        {
            syslog(LOG_ERR, "[TRANSFER] Failed to fork backup process for %s", filename);
            continue;
        }
        // Child process: Backup reports
        if (backup_pid == 0)
        {
            copy_report(source_path, BACKUP_DIR);
        }

        // Parent process: Auto backup & transfer reports
        waitpid(transfer_pid, NULL, 0);
        waitpid(backup_pid, NULL, 0);

        report_status[report_index]++;

        pid_t clean_pid = fork();

        if (clean_pid < 0)
        {
            syslog(LOG_ERR, "[TRANSFER] Failed to fork cleaning process for %s", filename);
            continue;
        }
        // Child process: Clean report
        if (clean_pid == 0)
        {
            remove_report(source_path);
            exit(EXIT_SUCCESS);
        }

        waitpid(clean_pid, NULL, 0);
    }

    for (int i = 0; i < NO_OF_DEPTS; i++)
    {
        job_status += report_status[i];

        if (report_status[i] == 0)
        {
            syslog(LOG_NOTICE, "[TRANSFER] %s not found in %s", report_names[i], UPLOAD_DIR);
        }
    }

    if (job_status == NO_OF_DEPTS)
    {
        syslog(LOG_INFO, "[TRANSFER] All reports are transferred and backed-up successfully");
    }

    exit(EXIT_SUCCESS);
}

void manual_backup_transfer_reports()
{
    const char *report_prefixes[] = {REPORT_PREFIXES};
    int report_count = 0;

    DIR *dir = opendir(UPLOAD_DIR);
    struct dirent *entry;

    if (dir == NULL)
    {
        syslog(LOG_ERR, "[TRANSFER] Failed to open %s: %m", UPLOAD_DIR);
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        const char *filename = entry->d_name;
        if (is_allowed_prefix(filename, report_prefixes) && is_xml_file(filename))
        {
            char source_path[256];

            snprintf(source_path, sizeof(source_path), "%s/%s", UPLOAD_DIR, filename);

            pid_t transfer_pid = fork();

            if (transfer_pid < 0)
            {
                syslog(LOG_ERR, "[TRANSFER] Failed to fork transfer process for %s", filename);
                continue;
            }
            // Child process: Transfer reports
            if (transfer_pid == 0)
            {
                copy_report(source_path, REPORTING_DIR);
            }

            pid_t backup_pid = fork();

            if (backup_pid < 0)
            {
                syslog(LOG_ERR, "[TRANSFER] Failed to fork backup process for %s", filename);
                continue;
            }
            // Child process: Backup reports
            if (backup_pid == 0)
            {
                copy_report(source_path, BACKUP_DIR);
            }

            // Parent process: Auto backup & transfer reports
            waitpid(transfer_pid, NULL, 0);
            waitpid(backup_pid, NULL, 0);

            report_count++;

            pid_t clean_pid = fork();

            if (clean_pid < 0)
            {
                syslog(LOG_ERR, "[TRANSFER] Failed to fork cleaning process for %s", filename);
                continue;
            }
            // Child process: Clean report
            if (clean_pid == 0)
            {
                remove_report(source_path);
                exit(EXIT_SUCCESS);
            }

            waitpid(clean_pid, NULL, 0);
        }
        else
        {
            syslog(LOG_WARNING, "[TRANSFER] Unknown file %s in %s: not follow file naming convention", filename, UPLOAD_DIR);
            continue;
        }
    }

    syslog(LOG_INFO, "[TRANSFER] %d report(s) are transferred and backed-up successfully", report_count);

    exit(EXIT_SUCCESS);
}