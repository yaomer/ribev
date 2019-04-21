#ifndef _RIBEV_CONFIG_H
#define _RIBEV_CONFIG_H

#if defined(__APPLE__)
// #define RB_HAVE_KQUEUE 1
#define RB_HAVE_POLL 1
#define RB_HAVE_PTHREAD_MACH_THREAD_NP 1
#endif
#if defined (__linux__)
#define RB_HAVE_EPOLL 1
#define RB_HAVE_POLL 1
#include <sys/syscall.h>
#define RB_HAVE_GETTID 1
#endif

#endif /* _RIBEV_CONFIG_H */
