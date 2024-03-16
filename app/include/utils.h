#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include "config.h"

void report_name_today(char *, int, char *, struct tm);
int check_upload(struct tm);

#endif // UTILS_H