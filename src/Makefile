PLATFORM = $(shell ./systype.sh)
CFLAGS = -Wall

ifeq "$(PLATFORM)" "linux"
    CC = gcc
    OPTION_OBJS = epoll.o poll.o
endif
ifeq "$(PLATFORM)" "macos"
    CC = clang
    OPTION_OBJS = kqueue.o poll.o
endif

all: libribev-serv.a libribev-cli.a

COMMON_OBJS = evloop.o \
			  channel.o \
			  buffer.o \
			  event.o \
			  evop.o \
			  timer.o \
			  task.o \
			  hash.o \
			  queue.o \
			  vector.o \
			  net.o \
			  log.o \
			  alloc.o \
			  $(OPTION_OBJS)

SERV_OBJS = server.o evll.o evthr.o $(COMMON_OBJS)

CLI_OBJS = client.o $(COMMON_OBJS)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

libribev-serv.a : $(SERV_OBJS)
	ar -r libribev-serv.a $(SERV_OBJS) 
	chmod 0777 libribev-serv.a
	
libribev-cli.a : $(CLI_OBJS)
	ar -r libribev-cli.a $(CLI_OBJS)
	chmod 0777 libribev-cli.a

.PHONY : clean
clean:
	-rm *.o
