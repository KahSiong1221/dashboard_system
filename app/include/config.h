#ifndef CONFIG_H
#define CONFIG_H

// Syslog identification string
#define LOG_IDENT "ss_daemon"

#define STORAGE_DIR "/home/ss_storage/"
// Shared upload directory path
#define UPLOAD_DIR "/home/ss_storage/upload/"
// Backup directory path
#define BACKUP_DIR "/home/ss_storage/backup/"
// Reporting directory path
#define REPORTING_DIR "/home/ss_storage/reporting/"

#define REPORT_PREFIXES "warehouse_report",     \
                        "manufacturing_report", \
                        "sales_report",         \
                        "distribution_report"

#define NO_OF_DEPTS 4

#define REPORT_FILE_EXT ".xml"

#endif // CONFIG_H
