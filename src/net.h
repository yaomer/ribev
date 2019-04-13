#ifndef _RIBEV_NET_H
#define _RIBEV_NET_H

long rb_thread_id(void);
void rb_set_nonblock(int fd);
void rb_socketpair(int *fd);
int rb_connect(int port, const char *addr);
int rb_listen(int port);
int rb_accept(int listenfd);

#endif /* _RIBEV_NET_H */
