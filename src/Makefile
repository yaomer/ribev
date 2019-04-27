PLATFORM = $(shell ./systype.sh)
LIBSERV = libribev-serv.a
LIBCLI = libribev-cli.a
CFLAGS = -Wall -std=c11

ifeq "$(PLATFORM)" "linux"
    CC = gcc
    OPTION_OBJS = epoll.o poll.o
endif
ifeq "$(PLATFORM)" "macos"
    CC = clang
    OPTION_OBJS = kqueue.o poll.o
endif

all : $(LIBSERV) $(LIBCLI)

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
			  util.o \
			  $(OPTION_OBJS)

SERV_OBJS = $(COMMON_OBJS) \
			server.o \
			evll.o \
			evthr.o

CLI_OBJS = $(COMMON_OBJS) \
		   client.o

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(LIBSERV) : $(SERV_OBJS)
	ar -r $(LIBSERV) $(SERV_OBJS) 
	
$(LIBCLI) : $(CLI_OBJS)
	ar -r $(LIBCLI) $(CLI_OBJS)

.PHONY : clean
clean:
	-rm *.o
