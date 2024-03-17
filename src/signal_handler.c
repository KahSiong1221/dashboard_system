#include "signal_handler.h"

void signal_handler(int sig)
{
    switch(sig)
    {
        case SIGTERM:
            syslog(LOG_NOTICE, "Daemon shut down");
            closelog();
            exit(EXIT_SUCCESS);
            break;
        case SIGUSR1:
            syslog(LOG_INFO, "Perform transfer task manually");
    }
}