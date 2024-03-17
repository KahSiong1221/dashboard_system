#ifndef DIR_MONITOR_H
#define DIR_MONITOR_H

#include <stdlib.h>
#include <errno.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include "config.h"

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUFFER_LEN (1024 * (EVENT_SIZE + 16))

void dir_monitor_test();

#endif // DIR_MONITOR_H