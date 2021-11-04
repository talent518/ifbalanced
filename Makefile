CC := gcc
CFLAGS := -O2 -fPIC
LFLAGS := -ldl

all: libifbalanced.so

test: libifbalanced.so
	@LD_PRELOAD=$(PWD)/libifbalanced.so curl -v baidu.com

libifbalanced.so: ifbalanced.o
	@echo LD $@
	@$(CC) -shared -o $@ $^ $(LFLAGS)

%.o: %.c
	@echo CC $<
	@$(CC) $(CFLAGS) -c $< -o $@

ifbalanced.o: ifbalanced.h

clean:
	@rm -vf *.o *.e libifbalanced.so
