#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <errno.h>
#include "evloop.h"
#include "logger.h"
#include "vector.h"
#include "buffer.h"
#include "lock.h"
#include "net.h"

char filename[PATH_MAX + 1];

rb_log_t *rb_log;

static void
rb_log_quit(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    pthread_exit(NULL);
}

char *
rb_get_time_fmtstr(int64_t ms, char *buf, int len)
{
    struct tm tm;
    time_t seconds = ms / 1000;
    gmtime_r(&seconds, &tm);
    snprintf(buf, len, "%4d-%02d-%02d-%02d:%02d:%02d",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec);
    return buf;
}

static void
rb_log_creat_dir(void)
{
    DIR *dir = opendir(".log");

    if (!dir && mkdir(".log", GV_LOG_DIR_MODE) < 0)
        rb_log_quit("mkdir error");
    if (dir)
        closedir(dir);
}

/*
 * filename[yy-mm-dd.pid.log]
 */
static int
rb_log_creat_file(void)
{
    int fd;
    size_t len;

    bzero(filename, sizeof(filename));
    strcpy(filename, ".log/");
    rb_get_time_fmtstr(rb_now(), filename + 5, 11);
    len = strlen(filename);
    snprintf(filename + len, PATH_MAX - len, ".%ld.log", (long)getpid());

    if ((fd = open(filename, GV_LOG_FILE_OPEN_MODE, GV_LOG_FILE_MODE)) < 0)
        rb_log_quit("open error");

    return fd;
}

static void
rb_log_wakeup(void **argv)
{
    rb_lock(&_log->mutex);
    _log->wakeup = 1;
    rb_unlock(&_log->mutex);
    rb_notify(&_log->cond);
}

static void
rb_log_write_to_buffer(char *s)
{
    rb_lock(&_log->mutex);
    rb_buffer_write(_log->buf, s, strlen(s));
    rb_unlock(&_log->mutex);
    if (GV_BUFFER_READABLE(_log->buf) >= GV_LOG_BUFSIZE)
        rb_notify(&_log->cond);
}

/*
 * 将[log buffer]中的数据全部写到fd中
 */
static void
rb_log_write_fd(int fd)
{
    ssize_t n;
    size_t readable = GV_BUFFER_READABLE(_log->buf);
    char *buf = GV_BUFFER_BEGIN(_log->buf);

    while ((n = write(fd, buf, readable)) < readable) {
        if (n < 0)
            rb_log_quit("write error");
        else {
            readable -= n;
            rb_buffer_update_ridx(_log->buf, n);
        }
    }
    rb_buffer_update_ridx(_log->buf, n);
}

static void *
rb_log_write_to_file(void *arg)
{
    struct stat statbuf;

    int fd = rb_log_creat_file();
    while (1) {
        rb_lock(&_log->mutex);
        while (!_log->quit && !_log->wakeup &&
                GV_BUFFER_READABLE(_log->buf) < GV_LOG_BUFSIZE)
            rb_wait(&_log->cond, &_log->mutex);

        if (lstat(filename, &statbuf) < 0)
            rb_log_quit("lstat error for %s", filename);
        if (statbuf.st_size > GV_LOG_FILESIZE) {
            close(fd);
            fd = rb_log_creat_file();
        }
        rb_log_write_fd(fd);

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
 * 每隔1秒冲洗一次[log buffer]
 */
void
rb_log_flush(Evloop *loop)
{
    Task *task = rb_alloc_task(0);
    task->callback = rb_log_wakeup;
    rb_run_every(loop, 1000, task);
}

void
rb_log_init(void)
{
    if (_log)
        rb_log_quit("more than one log in process");
    assert(_log = malloc(sizeof(Log)));
    _log->buf = rb_buffer_init();
    _log->quit = 0;
    _log->mask = 0;
    _log->wakeup = 0;
    rb_lock_init(&_log->mutex);
    rb_cond_init(&_log->cond);
    rb_log_creat_dir();
    pthread_create(&_log->tid, NULL, rb_log_write_to_file, NULL);
}

/*
 * fmt[[level] [time] [tid] [file] [line] [msg]]
 */
static void
rb_log_fmt(char *level, const char *_FILE, int _LINE,
        char *fmt, va_list ap)
{
    ssize_t len;
    char buf[GV_LOG_BUFSIZE + 2];

    snprintf(buf, GV_LOG_BUFSIZE, "%-5s %s ", level,
            rb_get_time_fmtstr(rb_now(), buf + 6, GV_LOG_BUFSIZE - 6));
    len = strlen(buf);
    snprintf(buf + len, GV_LOG_BUFSIZE - len, "%ld %s:%d ",
            rb_thread_id(), _FILE, _LINE);
    len = strlen(buf);
    vsnprintf(buf + len, GV_LOG_BUFSIZE - len, fmt, ap);
    strcat(buf, "\n");
    rb_log_write_to_buffer(buf);
}

void
rb_log_debug(const char *_FILE, int _LINE, char *fmt, ...)
{
    va_list ap;

    if (!_log || (_log->mask & LOG_DEBUG))
        return;
    va_start(ap, fmt);
    rb_log_fmt("DEBUG", _FILE, _LINE, fmt, ap);
    va_end(ap);
}

void
rb_log_warn(const char *_FILE, int _LINE, char *fmt, ...)
{
    va_list ap;

    if (!_log || (_log->mask & LOG_WARN))
        return;
    va_start(ap, fmt);
    rb_log_fmt("WARN", _FILE, _LINE, fmt, ap);
    va_end(ap);
}

void
rb_log_error(const char *_FILE, int _LINE, char *fmt, ...)
{
    va_list ap;

    if (!_log || (_log->mask & LOG_ERROR))
        return;
    va_start(ap, fmt);
    rb_log_fmt("ERROR", _FILE, _LINE, fmt, ap);
    va_end(ap);
    exit(1);
}

void
rb_log_set_mask(int level)
{
    _log->mask |= level;
}

void
rb_log_unset_mask(int level)
{
    _log->mask &= ~level;
}

void
rb_log_ban(void)
{
    rb_log_set_mask(LOG_DEBUG | LOG_WARN | LOG_ERROR);
}

void
rb_log_destroy(void)
{
    _log->quit = 1;
    rb_notify(&_log->cond);
    pthread_join(_log->tid, NULL);
    rb_lock_destroy(&_log->mutex);
    rb_cond_destroy(&_log->cond);
    rb_buffer_destroy(&_log->buf);
    free(_log);
    _log = NULL;
}
