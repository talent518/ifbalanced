#ifndef _IFBALANCED_H
#define _IFBALANCED_H

#include <unistd.h>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define gdecl(t,v) t v
#define gprintf(fmt, args...) fprintf(stderr, "[INFO][%s:%d] " fmt "\n", __FILE__, __LINE__, ##args)
#define gerror(fmt, args...) fprintf(stderr, "[ERR][%s:%d] " fmt "\n", __FILE__, __LINE__, ##args)

#undef satosin
#define satosin(x) ((struct sockaddr_in *) x)
#undef satosin6
#define satosin6(x) ((struct sockaddr_in6 *) x)
#define addr2str(x, b) (satosin(x)->sin_family == AF_INET ? inet_ntop(AF_INET, &satosin(x)->sin_addr, b, sizeof(b)) : inet_ntop(AF_INET6, &satosin6(x)->sin6_addr, b, sizeof(b)))
#define addr2port(x) (satosin(x)->sin_family == AF_INET ? ntohs(satosin(x)->sin_port) : ntohs(satosin6(x)->sin6_port))

typedef int (*connect_t)(int, const struct sockaddr *, socklen_t);
typedef ssize_t (*sendto_t) (int, const void *, size_t, int, const struct sockaddr *, socklen_t);
typedef int (*close_t)(int);
typedef struct hostent* (*gethostbyname_t)(const char *);
typedef int (*freeaddrinfo_t)(struct addrinfo *);
typedef struct hostent *(*gethostbyaddr_t) (const void *, socklen_t, int);
typedef int (*getaddrinfo_t)(const char *, const char *, const struct addrinfo *, struct addrinfo **);
typedef int (*getnameinfo_t) (const struct sockaddr *, socklen_t, char *, socklen_t, char *, socklen_t, int);

extern connect_t true_connect;
extern gethostbyname_t true_gethostbyname;
extern getaddrinfo_t true_getaddrinfo;
extern freeaddrinfo_t true_freeaddrinfo;
extern getnameinfo_t true_getnameinfo;
extern gethostbyaddr_t true_gethostbyaddr;

#endif
