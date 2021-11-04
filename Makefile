CC := gcc
CFLAGS := -O2 -fPIC
LFLAGS := -ldl

ifneq ($(KLOG),)
CFLAGS += -DHAVE_KLOG=1
endif

all: libifbalanced.so

test: libifbalanced.so
	@LD_PRELOAD=$(PWD)/libifbalanced.so curl -v baidu.com

libifbalanced.so: ifbalanced.o klog.o
	@echo LD $@
	@$(CC) -shared -o $@ $^ $(LFLAGS)

%.o: %.c
	@echo CC $<
	@$(CC) $(CFLAGS) -c $< -o $@

ifbalanced.o: ifbalanced.h
klog.o: klog.h

clean:
	@rm -vf *.o *.e libifbalanced.so
