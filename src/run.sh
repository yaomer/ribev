#/bin/sh

cc server.c alloc.c buffer.c channel.c coder.c event.c evloop.c evop.c hash.c \
    logger.c timer.c task.c kqueue.c epoll.c net.c queue.c vector.c -o serv
cc client.c alloc.c buffer.c channel.c coder.c event.c evloop.c evop.c hash.c \
    logger.c timer.c task.c kqueue.c epoll.c net.c queue.c vector.c -o cli
