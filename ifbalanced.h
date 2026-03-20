#ifndef _IFBALANCED_H
#define _IFBALANCED_H

#include <syslog.h>

#ifdef USE_GDEBUG
#   define gdebug(fmt, args...) syslog(LOG_DEBUG, "[%d][%s][%s] " fmt "\n", getppid(), comm, __func__, ##args)
#else
#   define gdebug(fmt, args...)
#endif

#define gprintf(fmt, args...) syslog(LOG_INFO, "[%d][%s][%s] " fmt "\n", getppid(), comm, __func__, ##args)
#define gerror(fmt, args...) syslog(LOG_ERR, "[%d][%s][%s] " fmt "\n", getppid(), comm, __func__, ##args)

extern char comm[];

#endif
