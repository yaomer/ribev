#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include "evloop.h"
#include "timer.h"
#include "log.h"
#include "vector.h"
#include "buffer.h"
#include "lock.h"
#include "log_fmt.h"

rb_log_t *_log;

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

        ssize_t readable = rb_buffer_readable(_log->buf);
        write(fd, rb_buffer_begin(_log->buf), readable);
        rb_buffer_update_readidx(_log->buf, readable);

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
    pthread_create(&_log->tid, NULL, rb_log_write_to_file, NULL);
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
