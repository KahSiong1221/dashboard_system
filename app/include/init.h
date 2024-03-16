#ifndef INIT_H
#define INIT_H

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <grp.h>
#include "config.h"

void mkdir_if_not_exists(char *, mode_t);

void daemon_init();

#endif // INIT_H