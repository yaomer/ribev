#ifdef RB_HAVE_GETTID
#include <sys/syscall.h>
#endif
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include "config.h"
#include "net.h"
#include "log.h"

long
rb_thread_id(void)
{
#ifdef RB_HAVE_PTHREAD_MACH_THREAD_NP
    return pthread_mach_thread_np(pthread_self());
#elif RB_HAVE_GETTID
    return syscall(SYS_gettid);
#else
    rb_log_error("can't return tid");
    return -1;
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
        rb_log_error("socketpair");
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
            rb_log_error("fd=%d connected timeout", connfd);
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
                rb_log_error("getsockopt: fd=%d, err=%d", connfd, err);
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

    if (signal(SIGPIPE, SIG_IGN) < 0)
        rb_log_error("signal");

    if ((connfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        rb_log_error("socket");

    rb_set_nonblock(connfd);

    bzero(&cliaddr, sizeof(cliaddr));
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, addr, (void *)&cliaddr.sin_addr) <= 0)
        rb_log_error("inet_pton");

    if (connect(connfd, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0) {
        if (errno == EINPROGRESS) {
            tv.tv_sec = 2;
            tv.tv_usec = 0;
            connfd = rb_connect_retry(connfd, &tv);
        } else
            rb_log_error("connect: fd=%d", connfd);
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
        rb_log_error("signal");

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        rb_log_error("socket");

    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
        rb_log_error("setsockopt: set SO_REUSEADDR");

    if (setsockopt(listenfd, SOL_SOCKET, TCP_NODELAY, &on, sizeof(on)) < 0)
        rb_log_error("setsockopt: set TCP_NODELAY");

    rb_set_nonblock(listenfd);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        rb_log_error("bind");

    if (listen(listenfd, RB_LISTENQ) < 0)
        rb_log_error("listen");

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
        rb_log_error("listenfd=%d accept fd=%d", listenfd, connfd);

    return connfd;
}
