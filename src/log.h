#ifndef __LOG_H__
#define __LOG_H__

#include <syslog.h>

void _logf( int prio, const char * format, ... );

#endif
