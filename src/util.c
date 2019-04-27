#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <inttypes.h>
#include <pthread.h>
#include "config.h"
#include "util.h"
#ifdef RB_HAVE_GETTID
#include <sys/syscall.h>
#endif

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

/*
 * 返回当前日历时间的时间戳(ms)
 */
int64_t
rb_now(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/*
 * 获取当前(GMT)时间的字符串表示，精确到ms
 * len(24)
 */
char *
rb_get_timestr(int64_t ms, char *buf, size_t len)
{
    struct tm tm;
    time_t seconds = ms / 1000;
    gmtime_r(&seconds, &tm);
    snprintf(buf, len, "%4d-%02d-%02d-%02d:%02d:%02d.%04lld",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec, ms % 1000);
    return buf;
}

/*
 * 获取本地时间
 */
char *
rb_local_timestr(char *buf, size_t len)
{
    time_t tm = time(NULL);
    ctime_r(&tm, buf);
    return buf;
}
