CC := gcc
CFLAGS := -O2 -fPIC
LFLAGS := -ldl -pthread

ifneq ($(KLOG),)
CFLAGS += -DHAVE_KLOG=1
endif

ifneq ($(BIND),)
CFLAGS += -DUSE_BIND=1
endif

all: libifbalanced.so

test: libifbalanced.so
	@LD_PRELOAD=$(PWD)/libifbalanced.so IFBALANCED_FILE=ifbalanced.conf curl -s --connect-timeout 3 -v -4 -L baike.baidu.com news.baidu.com

libifbalanced.so: ifbalanced.o klog.o api.o
	@echo LD $@
	@$(CC) -shared -o $@ $^ $(LFLAGS)

%.o: %.c
	@echo CC $<
	@$(CC) $(CFLAGS) -c $< -o $@

ifbalanced.o: ifbalanced.h klog.h api.h
klog.o: klog.h
api.o: api.h

clean:
	@rm -vf *.o *.e libifbalanced.so
