#undef _GNU_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <netdb.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <pthread.h>

#include "ifbalanced.h"

connect_t true_connect;
sendto_t true_sendto;
close_t true_close;
gethostbyname_t true_gethostbyname;
getaddrinfo_t true_getaddrinfo;
freeaddrinfo_t true_freeaddrinfo;
getnameinfo_t true_getnameinfo;
gethostbyaddr_t true_gethostbyaddr;

pthread_once_t init_once = PTHREAD_ONCE_INIT;

static void* load_sym(char* symname, void* proxyfunc) {

	void *funcptr = dlsym(RTLD_NEXT, symname);

	if(!funcptr) {
		gerror("Cannot load symbol '%s' %s", symname, dlerror());
		exit(1);
	} else {
		gprintf("loaded symbol '%s'" " real addr %p  wrapped addr %p", symname, funcptr, proxyfunc);
	}
	if(funcptr == proxyfunc) {
		gerror("circular reference detected, aborting!");
		abort();
	}
	return funcptr;
}

static void do_init(void);

/* if we use gcc >= 3, we can instruct the dynamic loader 
 * to call init_lib at link time. otherwise it gets loaded
 * lazily, which has the disadvantage that there's a potential
 * race condition if 2 threads call it before init_l is set 
 * and PTHREAD support was disabled */
#if __GNUC__ > 2
__attribute__((constructor))
static void gcc_init(void) {
	pthread_once(&init_once, do_init);
}
#endif

#define SETUP_SYM(X) do { \
	if(! true_##X) { \
		true_##X = load_sym(#X, X); \
	} \
} while(0)

static void do_init(void) {
	gprintf("network interface balanced init ...");

	SETUP_SYM(connect);
	SETUP_SYM(sendto);
	SETUP_SYM(close);
	SETUP_SYM(gethostbyname);
	SETUP_SYM(getaddrinfo);
	SETUP_SYM(freeaddrinfo);
	SETUP_SYM(gethostbyaddr);
	SETUP_SYM(getnameinfo);
}

int connect(int fd, const struct sockaddr *addr, unsigned int len) {
	gdecl(char, buf[256]);
	gprintf("connect %d to %s:%d", fd, addr2str(addr, buf), addr2port(addr));

	return true_connect(fd, addr, len);
}

ssize_t sendto(int fd, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen) {
	gdecl(char, buf2[256]);
	gprintf("sendto %d to %s:%d", fd, addr2str(addr, buf2), addr2port(addr));

	return true_sendto(fd, buf, len, flags, addr, addrlen);
}

int close(int fd) {
	gprintf("close %d", fd);

	return true_close(fd);
}

struct hostent *gethostbyname(const char *name) {
	gprintf("get host by name '%s'", name);

	return true_gethostbyname(name);
}

int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res) {
	int ret = true_getaddrinfo(node, service, hints, res);

	gprintf("get addr info by node '%s' and service '%s' and hints %p and res %p", node, service, hints, res ? *res : NULL);

	return ret;
}

void freeaddrinfo(struct addrinfo *res) {
	gprintf("free addr info by %p", res);

	true_freeaddrinfo(res);
}

struct hostent *gethostbyaddr(const void *addr, socklen_t len, int type) {
	gdecl(char, buf[256]);
	gprintf("get host by addr %s", inet_ntop(type, addr, buf, sizeof(buf)));

	return true_gethostbyaddr(addr, len, type);
}

int getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host, socklen_t hostlen, char *serv, socklen_t servlen, int flags) {
	gdecl(char, buf[256]);
	gprintf("get name info by addr %s:%d and host '%s' and service '%s' and flags %d", addr2str(sa, buf), addr2port(sa), host, serv, flags);

	return true_getnameinfo(sa, salen, host, hostlen, serv, servlen, flags);
}
