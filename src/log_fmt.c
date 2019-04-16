#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <errno.h>
#include "lock.h"
#include "timer.h"
#include "buffer.h"
#include "log.h"
#include "log_fmt.h"
#include "net.h"

static char file[PATH_MAX + 1];

/*
 * 获取当前时间的字符串表示，精确到ms
 */
static char *
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
 * 在当前目录下创建[.log]目录，以后所有日志文件将在
 * 该目录下创建。
 */
void
rb_log_creat_dir(void)
{
    DIR *dir = opendir(".log");

    if (!dir && mkdir(".log", 0777) < 0)
        ;
    if (dir)
        closedir(dir);
}

/*
 * 创建一个日志文件，格式：[yy-mm-dd.pid.log]
 */
int
rb_log_creat_file(void)
{
    int fd;
    size_t len;

    bzero(file, sizeof(file));
    strcpy(file, ".log/");

    rb_get_timestr(rb_now(), file + 5, 11);
    len = strlen(file);
    snprintf(file + len, PATH_MAX - len, ".%ld.log", (long)getpid());

    if ((fd = open(file, (O_RDWR | O_APPEND | O_CREAT), 0666)) < 0)
        ;

    return fd;
}

/*
 * 将日志消息写到[buffer]中
 */
static void
rb_log_write_to_buffer(char *s)
{
    rb_lock(&_log->mutex);
    rb_buffer_write(_log->buf, s, strlen(s));
    if (rb_buffer_readable(_log->buf) >= RB_LOG_BUFSIZE) {
        rb_unlock(&_log->mutex);
        rb_notify(&_log->cond);
    } else
        rb_unlock(&_log->mutex);
}

/*
 * 构造日志消息，格式：[level][time][tid][file][line][msg]
 */
void
rb_log_fmt(const char *level, const char *_FILE, int _LINE,
        const char *fmt, va_list ap)
{
    ssize_t len;
    char buf[RB_LOG_BUFSIZE + 2];

    snprintf(buf, RB_LOG_BUFSIZE, "%-5s %s ", level,
            rb_get_timestr(rb_now(), buf + 6, RB_LOG_BUFSIZE - 6));
    len = strlen(buf);
    snprintf(buf + len, RB_LOG_BUFSIZE - len, "%ld %s:%d ",
            rb_thread_id(), _FILE, _LINE);
    len = strlen(buf);
    vsnprintf(buf + len, RB_LOG_BUFSIZE - len, fmt, ap);
    strcat(buf, "\n");
    rb_log_write_to_buffer(buf);
}

const char *
rb_logstr(int level)
{
    if (level == RB_LOG_DEBUG)
        return "DEBUG";
    else if (level == RB_LOG_WARN)
        return "WARN";
    else if (level == RB_LOG_ERROR)
        return "ERROR";
    else
        return "none";
}
