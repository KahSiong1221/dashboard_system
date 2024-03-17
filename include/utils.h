#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include "config.h"

void report_name_today(char *, int, const char *, struct tm);
void check_upload(struct tm);
int lock_dir(char *);
int unlock_dir(char *, mode_t);
void auto_backup_transfer_reports(struct tm);
void copy_report(const char *, const char *);
void remove_report(const char *);
int is_report(const char *, char[NO_OF_DEPTS][100]);

#endif // UTILS_H