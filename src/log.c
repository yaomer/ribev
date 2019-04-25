#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <errno.h>
#include "evloop.h"
#include "timer.h"
#include "log.h"
#include "vector.h"
#include "buffer.h"
#include "lock.h"
#include "net.h"

rb_log_t *_log;

static char file[PATH_MAX + 1];

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
static void
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
static int
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
 * 将[log buffer]中的数据写到本地文件中。
 */
static void *
rb_log_write_to_file(void *arg)
{
    struct stat statbuf;

    int fd = rb_log_creat_file();
    while (1) {
        rb_lock(&_log->mutex);
        while (!_log->quit && !_log->wakeup &&
                rb_buffer_readable(_log->buf) < RB_LOG_BUFSIZE)
            rb_wait(&_log->cond, &_log->mutex);

        if (fstat(fd, &statbuf) < 0)
            ;
        if (statbuf.st_size > RB_LOG_FILESIZE) {
            close(fd);
            fd = rb_log_creat_file();
        }

        ssize_t n = write(fd, rb_buffer_begin(_log->buf), rb_buffer_readable(_log->buf));
        rb_buffer_retrieve(_log->buf, n);

        if (_log->quit) {
            rb_unlock(&_log->mutex);
            pthread_exit(NULL);
        }
        _log->wakeup = 0;
        rb_unlock(&_log->mutex);
    }
    return 0;
}

/*
 * 唤醒日志线程
 */
static void
rb_log_wakeup(void **argv)
{
    rb_lock(&_log->mutex);
    _log->wakeup = 1;
    rb_unlock(&_log->mutex);
    rb_notify(&_log->cond);
}

/*
 * 每隔3秒flush一次[log buffer]
 */
void
rb_log_flush(rb_evloop_t *loop)
{
    rb_task_t *t = rb_alloc_task(0);
    t->callback = rb_log_wakeup;
    rb_run_every(loop, 1000 * 3, t);
}

/*
 * 由于logger是全局共享的，所以每个进程只能持有一个logger
 */
void
rb_log_init(void)
{
    if (_log)
        ;
    assert(_log = malloc(sizeof(rb_log_t)));
    _log->buf = rb_buffer_init();
    _log->quit = 0;
    _log->mask = 0;
    _log->wakeup = 0;
    rb_lock_init(&_log->mutex);
    rb_cond_init(&_log->cond);
    rb_log_creat_dir();
    rb_creat_thread(&_log->tid, rb_log_write_to_file, NULL);
}

/*
 * 将日志消息写到[buffer]中
 */
static void
rb_log_write_to_buffer(char *s)
{
    ssize_t len = strlen(s);
    rb_lock(&_log->mutex);
    rb_buffer_move_forward(_log->buf, len);
    rb_buffer_write(_log->buf, s, len);
    if (rb_buffer_readable(_log->buf) >= RB_LOG_BUFSIZE) {
        rb_unlock(&_log->mutex);
        rb_notify(&_log->cond);
    } else
        rb_unlock(&_log->mutex);
}

/*
 * 构造日志消息，格式：[level][time][tid][file][line][msg]
 */
static void
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

void
rb_log_output(int level, const char *_FILE, int _LINE,
        const char *fmt, ...)
{
    va_list ap;

    if (!_log)
        return;
    if (_log->mask & level)
        return;
    va_start(ap, fmt);
    rb_log_fmt(rb_logstr(level), _FILE, _LINE, fmt, ap);
    va_end(ap);
    if (level & RB_LOG_ERROR)
        exit(1);
}

void
rb_log_set_mask(int level)
{
    if (level & RB_LOG_ERROR) {
        _log->mask |= RB_LOG_ERROR | RB_LOG_WARN | RB_LOG_DEBUG;
    } else if (level & RB_LOG_WARN) {
        _log->mask |= RB_LOG_WARN | RB_LOG_DEBUG;
    } else if (level & RB_LOG_DEBUG) {
        _log->mask |= RB_LOG_DEBUG;
    }
}

void
rb_log_unset_mask(int level)
{
    _log->mask &= ~level;
}
