#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <stdlib.h>
#include <syslog.h>
#include <signal.h>
#include "config.h"

void main_signal_handler(int);
void monitor_signal_handler(int);

#endif // SIGNAL_HANDLER_H