#ifndef _IFBALANCED_H
#define _IFBALANCED_H

#include <unistd.h>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define gdecl(t,v) t v
#ifdef HAVE_KLOG
#include "klog.h"
#define gprintf(fmt, args...) KLOG_INFO("ifBalanced", "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##args)
#define gerror(fmt, args...) KLOG_ERROR("ifBalanced", "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##args)
#else
#define gprintf(fmt, args...) fprintf(stderr, "[INFO][%s:%d] " fmt "\n", __FILE__, __LINE__, ##args)
#define gerror(fmt, args...) fprintf(stderr, "[ERR][%s:%d] " fmt "\n", __FILE__, __LINE__, ##args)
#endif

#endif
