#undef _GNU_SOURCE
#define _GNU_SOURCE

#undef __USE_MISC
#define __USE_MISC

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <netdb.h>

#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <pthread.h>
#include <math.h>

#include "ifbalanced.h"
#include "api.h"

#undef satosin
#define satosin(x) ((struct sockaddr_in *) x)
#undef satosin6
#define satosin6(x) ((struct sockaddr_in6 *) x)
#define addr2str(x, b) (satosin(x)->sin_family == AF_INET ? inet_ntop(AF_INET, &satosin(x)->sin_addr, b, sizeof(b)) : inet_ntop(AF_INET6, &satosin6(x)->sin6_addr, b, sizeof(b)))
#define addr2port(x) (satosin(x)->sin_family == AF_INET ? ntohs(satosin(x)->sin_port) : ntohs(satosin6(x)->sin6_port))

typedef int (*socket_t) (int, int, int);
typedef int (*connect_t) (int, const struct sockaddr *, socklen_t);
typedef ssize_t (*sendto_t) (int, const void *, size_t, int, const struct sockaddr *, socklen_t);
typedef int (*socketpair_t) (int, int, int, int[2]);
typedef int (*close_t) (int);

socket_t true_socket;
connect_t true_connect;
sendto_t true_sendto;
socketpair_t true_socketpair;
close_t true_close;

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
static void do_free(void);

/* if we use gcc >= 3, we can instruct the dynamic loader 
 * to call init_lib at link time. otherwise it gets loaded
 * lazily, which has the disadvantage that there's a potential
 * race condition if 2 threads call it before init_l is set 
 * and PTHREAD support was disabled */
#if __GNUC__ > 2
static pthread_once_t init_once = PTHREAD_ONCE_INIT;
__attribute__((constructor))
static void gcc_init(void) {
	pthread_once(&init_once, do_init);
}
static pthread_once_t free_once = PTHREAD_ONCE_INIT;
__attribute__((destructor))
static void gcc_free(void) {
	pthread_once(&free_once, do_free);
}
#endif

#define SETUP_SYM(X) do { \
	if(! true_##X) { \
		true_##X = load_sym(#X, X); \
	} \
} while(0)

typedef struct {
	char iface[16];
	in_addr_t addr;
	in_addr_t mask;
	in_addr_t net;
} privnet_t;

typedef struct {
	char iface[16];
	in_addr_t addr;
} pubnet_t;

static bool is_init = false;
static int npriv = 0, nprivs[16], iprivs[16];
static privnet_t privates[16][16];
static int npub = 0, ipub = 0;
static pubnet_t pubs[16];
static pthread_mutex_t mutex;

static void do_init(void) {
#ifdef HAVE_KLOG
	{
		char *level = getenv("IFBALANCED_LEVEL");
		if(level) klog_set_level(atoi(level));
	}
#endif
	gprintf("network interface balanced init ...");

	SETUP_SYM(socket);
	SETUP_SYM(connect);
	SETUP_SYM(sendto);
	SETUP_SYM(socketpair);
	SETUP_SYM(close);

	{
		FILE *fp;
		char *file, line[1024], *argv[32], addr[16], iface[16];
		int argc, rows = 0, i, mask;

		file = getenv("IFBALANCED_FILE");
		if(!file) file = "/etc/ifbalanced.conf";

		fp = fopen(file, "r");
		if(!fp) {
			gerror("open config file %s failure", file);
			return;
		}

		while(fgets(line, sizeof(line), fp)) {
			rows++;
			argc = make_cmd_args(line, argv, sizeof(argv)/sizeof(argv[0]));
			if(argc < 2) {
				if(argc > 0) {
					gerror("error in %d line of %s", rows, file);
					fclose(fp);
					return;
				} else {
					continue;
				}
			}

			if(!strcasecmp(argv[0], "public")) {
				for(i = 1; i < argc; i ++) {
					if(sscanf(argv[i], "%[^:]:%[0-9.]", iface, addr) != 2) {
						gerror("error in %d line of %s: %s", rows, file, argv[i]);
						fclose(fp);
						return;
					}

					pubnet_t *in = &pubs[npub];
					strcpy(in->iface, iface);
					in->addr = inet_addr(addr);
					npub ++;
				}
			} else if(!strcasecmp(argv[0], "private")) {
				nprivs[npriv] = 0;
				iprivs[npriv] = 0;
				for(i = 1; i < argc; i ++) {
					if(sscanf(argv[i], "%[^:]:%[0-9.]/%d", iface, addr, &mask) != 3) {
						gerror("error in %d line of %s: %s", rows, file, argv[i]);
						fclose(fp);
						return;
					}

					privnet_t *in = &privates[npriv][nprivs[npriv]];
					strcpy(in->iface, iface);
					in->mask = (1 << mask) - 1L;
					in->addr = inet_addr(addr);
					in->net = in->addr & in->mask;

					nprivs[npriv] ++;
				}
				npriv ++;
			} else {
				for(i=0; i<argc; i++) gerror("argv[%d] = %s", i, argv[i]);
			}
		}

		fclose(fp);
		pthread_mutex_init(&mutex, NULL);
		is_init = true;
	}
}

static void do_free(void) {
	gprintf("network interface balanced free ...");
	if(is_init) {
		pthread_mutex_destroy(&mutex);
	}
}

static void localaddr_bind(int fd, const struct sockaddr_in *addr, const char *func) {
	int i, j, idx;
	struct ifreq ifr;
	char buf[16];

	inet_ntop(AF_INET, &addr->sin_addr, buf, sizeof(buf));

	for(i = 0; i < npriv; i ++) {
		for(j = 0; j < nprivs[i]; j ++) {
			privnet_t *in = &privates[i][j];
			if((addr->sin_addr.s_addr & in->mask) == in->net) {
				pthread_mutex_lock(&mutex);
				idx = iprivs[i];
				in = &privates[i][idx];
				pthread_mutex_unlock(&mutex);

			#ifdef USE_BIND
				struct sockaddr_in sa;
				sa.sin_family = AF_INET;
				sa.sin_addr.s_addr = in->addr;
				sa.sin_port = 0;
				if(bind(fd, &sa, sizeof(sa)) == 0) {
			#else
				strcpy(ifr.ifr_name, in->iface);
				if(setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) == 0) {
			#endif
					gprintf("private %d:%d bind interface %s => %s ok", i, idx, in->iface, buf);

					pthread_mutex_lock(&mutex);
					iprivs[i] ++;
					if(iprivs[i] >= nprivs[i]) {
						iprivs[i] = 0;
					}
					pthread_mutex_unlock(&mutex);
				} else {
					gerror("private %d:%d bind interface %s => %s error: %s", i, idx, in->iface, buf, strerror(errno));
				}
				return;
			}
		}
	}

	if(npub > 0) {
		pubnet_t *in;
		pthread_mutex_lock(&mutex);
		in = &pubs[ipub];
		pthread_mutex_unlock(&mutex);

	#ifdef USE_BIND
		struct sockaddr_in sa;
		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = in->addr;
		sa.sin_port = 0;
		if(bind(fd, &sa, sizeof(sa)) == 0) {
	#else
		strcpy(ifr.ifr_name, in->iface);
		if(setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) == 0) {
	#endif
			gprintf("public %d bind interface %s => %s ok", ipub, in->iface, buf);

			pthread_mutex_lock(&mutex);
			ipub ++;
			if(ipub >= npub) ipub = 0;
			pthread_mutex_unlock(&mutex);
		} else {
			pthread_mutex_lock(&mutex);
			gerror("public %d bind interface %s => %s error: %s", ipub, in->iface, buf, strerror(errno));
			pthread_mutex_unlock(&mutex);
		}
	}
}

int socket(int domain, int type, int protocol) {
	int fd = true_socket(domain, type, protocol);

	if(fd >= 0 && domain == AF_INET) {
		gprintf("socket %d", fd);
	}

	return fd;
}

int connect(int fd, const struct sockaddr *addr, unsigned int len) {
	if(is_init && satosin(addr)->sin_family == AF_INET) {
		gdecl(char, buf[256]);
		gprintf("connect %d to %s:%d", fd, addr2str(addr, buf), addr2port(addr));

		localaddr_bind(fd, satosin(addr), __func__);
	}

	return true_connect(fd, addr, len);
}

ssize_t sendto(int fd, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen) {
	if(is_init && satosin(addr)->sin_family == AF_INET) {
		gdecl(char, buf2[256]);
		gprintf("sendto %d to %s:%d", fd, addr2str(addr, buf2), addr2port(addr));

		localaddr_bind(fd, satosin(addr), __func__);
	}

	return true_sendto(fd, buf, len, flags, addr, addrlen);
}

int socketpair(int domain, int type, int protocol, int fds[2]) {
	int ret = true_socketpair(domain, type, protocol, fds);

	if(ret == 0 && domain == AF_INET) {
		gprintf("socketpair %d %d", fds[0], fds[1]);
	}

	return ret;
}

int close(int fd) {
	if(is_init && isfdtype(fd, S_IFSOCK) > 0) {
		gprintf("close %d", fd);
	}

	return true_close(fd);
}
