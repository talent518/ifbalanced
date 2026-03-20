CC := gcc
CFLAGS := -O2 -fPIC
LFLAGS := -ldl -pthread

ifneq ($(BIND),)
CFLAGS += -DUSE_BIND=1
endif

all: libifbalanced.so

install: libifbalanced.so
	@sudo cp -vf ifbalanced.conf /etc/
	@sudo cp -vf libifbalanced.so /usr/local/lib/
	@echo -n "alias ifbalanced='LD_PRELOAD=/usr/local/lib/libifbalanced.so'" | sudo tee /etc/profile.d/ifbalanced.sh > /dev/null
	@echo Add ifbalanced command alias to /etc/profile.d/ifbalanced.sh

uninstall:
	@sudo rm -vf /etc/ifbalanced.conf /usr/local/lib/libifbalanced.so /etc/profile.d/ifbalanced.sh

test: libifbalanced.so
	@LD_PRELOAD=$(PWD)/libifbalanced.so IFBALANCED_FILE=ifbalanced.conf curl -s --connect-timeout 3 -v -4 -L baike.baidu.com news.baidu.com

libifbalanced.so: ifbalanced.o api.o
	@echo LD $@
	@$(CC) -shared -o $@ $^ $(LFLAGS)

%.o: %.c
	@echo CC $<
	@$(CC) $(CFLAGS) -c $< -o $@

ifbalanced.o: ifbalanced.h api.h
api.o: api.h

clean:
	@rm -vf *.o *.e libifbalanced.so
