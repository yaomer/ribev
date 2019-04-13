#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include "net.h"

long
rb_thread_id(void)
{
#ifdef __linux__
    /* syscall(SYS_gettid) */
    return (long)gettid();
#elif __mac__
    return pthread_mach_thread_np(pthread_self());
#else
    ; /* error */
#endif
}

void
rb_set_nonblock(int fd)
{
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

void
rb_socketpair(int *fd)
{
    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, fd) < 0)
        ;
}

#define RB_CONNECT_RETRY_MAXTIME 32

static int
rb_connect_retry(int connfd, struct timeval *tv)
{
    int ret;
    fd_set rdset, wrset;

    FD_ZERO(&rdset);
    FD_ZERO(&wrset);
    FD_SET(connfd, &rdset);
    FD_SET(connfd, &wrset);
retry:
    if ( (ret = select(connfd + 1, &rdset, &wrset, NULL, tv)) < 0)
        ;
    else if (ret == 0) {
        if (tv->tv_sec >= RB_CONNECT_RETRY_MAXTIME)
            ;
        else {
            tv->tv_sec *= 2;
            tv->tv_usec = 0;
        }
        goto retry;
    }
    if (FD_ISSET(connfd, &wrset)) {
        if (FD_ISSET(connfd, &rdset)) {
            socklen_t err;
            socklen_t len = sizeof(err);
            if ((ret = getsockopt(connfd, SOL_SOCKET, SO_ERROR, &err, &len)) < 0)
                ;
        }
    }
    rb_set_nonblock(connfd);
    return connfd;
}

int
rb_connect(int port, const char *addr)
{
    int connfd;
    struct sockaddr_in cliaddr;
    struct timeval tv;

    if ((connfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        ;

    rb_set_nonblock(connfd);

    bzero(&cliaddr, sizeof(cliaddr));
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, addr, (void *)&cliaddr.sin_addr) <= 0)
        ;

    if (connect(connfd, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0) {
        if (errno == EINPROGRESS) {
            tv.tv_sec = 2;
            tv.tv_usec = 0;
            connfd = rb_connect_retry(connfd, &tv);
        } else
            ;
    }

    return connfd;
}

int
rb_listen(int port)
{
    int listenfd;
    struct sockaddr_in servaddr;
    socklen_t on = 1;

    if (signal(SIGPIPE, SIG_IGN) < 0)
        ;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        ;

    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
        ;

    if (setsockopt(listenfd, SOL_SOCKET, TCP_NODELAY, &on, sizeof(on)) < 0)
        ;

    rb_set_nonblock(listenfd);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        ;

    if (listen(listenfd, 16) < 0)
        ;
    return listenfd;
}

int
rb_accept(int listenfd)
{
    int connfd;
    struct sockaddr_in cliaddr;
    socklen_t clilen;

    clilen = sizeof(cliaddr);
    if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0)
        ;

    return connfd;
}
