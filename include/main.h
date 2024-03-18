#ifndef MAIN_H
#define MAIN_H

#include <stdlib.h>
#include <syslog.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include "init.h"
#include "config.h"
#include "utils.h"
#include "dir_monitor.h"

void signal_handler(int);
void run_backup_transfer_reports();
void daemon_work();
int main(int, char *[]);

#endif // MAIN_H