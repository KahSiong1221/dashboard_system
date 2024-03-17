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
            syslog(LOG_WARNING, "[TRANSFER] Unknown file %s in %s: not follow file naming convention of report is out of date", filename, UPLOAD_DIR);
            continue;
        }

        char source_path[256];
        char backup_path[256];
        char reporting_path[256];

        snprintf(source_path, sizeof(source_path), "%s/%s", UPLOAD_DIR, filename);
        snprintf(backup_path, sizeof(backup_path), "%s/%s", BACKUP_DIR, filename);
        snprintf(reporting_path, sizeof(reporting_path), "%s/%s", REPORTING_DIR, filename);

        pid_t transfer_pid = fork();
        int transfer_status;

        if (transfer_pid < 0)
        {
            syslog(LOG_ERR, "[TRANSFER] Failed to fork transfer process for %s", filename);
            continue;
        }
        // Child process: Transfer reports
        if (transfer_pid == 0)
        {
            copy_report(source_path, reporting_path);
            exit(EXIT_SUCCESS);
        }

        pid_t backup_pid = fork();
        int backup_status;

        if (backup_pid < 0)
        {
            syslog(LOG_ERR, "[TRANSFER] Failed to fork backup process for %s", filename);
            continue;
        }
        // Child process: Backup reports
        if (backup_pid == 0)
        {
            copy_report(source_path, backup_path);
            exit(EXIT_SUCCESS);
        }

        // Parent process: Auto backup & transfer reports
        waitpid(transfer_pid, &transfer_status, 0);
        waitpid(backup_pid, &backup_status, 0);

        syslog(LOG_DEBUG, "[DEBUG] transfer_status: %d", WEXITSTATUS(transfer_status));
        if (WIFEXITED(transfer_status) && WEXITSTATUS(transfer_status) == 0)
        {
            report_status[report_index]++;
        }
        syslog(LOG_DEBUG, "[DEBUG] backup_status: %d", WEXITSTATUS(backup_status));

        if (WIFEXITED(backup_status) && WEXITSTATUS(backup_status))
        {
            report_status[report_index] += 2;
        }

        syslog(LOG_DEBUG, "[DEBUG] report status: %d %d %d %d", report_status[0], report_status[1], report_status[2], report_status[3]);

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

    syslog(LOG_DEBUG, "[DEBUG] report status: %d %d %d %d", report_status[0], report_status[1], report_status[2], report_status[3]);

    for (int i = 0; i < NO_OF_DEPTS; i++)
    {
        job_status += report_status[i];

        if (report_status[i] == 0)
        {
            syslog(LOG_NOTICE, "[TRANSFER] %s not found in %s", report_names[i], UPLOAD_DIR);
        }
    }

    if (job_status == 3 * NO_OF_DEPTS)
    {
        syslog(LOG_INFO, "[TRANSFER] All reports are transferred and backed-up successfully");
    }

    exit(EXIT_SUCCESS);
}