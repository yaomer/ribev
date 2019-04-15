#ifndef _RIBEV_LOGGER_H
#define _RIBEV_LOGGER_H

#include <pthread.h>
#include "fwd.h"

typedef struct rb_log {
    rb_buffer_t *buf;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_t tid;
    int wakeup;
    int quit;
    int mask;
} rb_log_t;

enum RB_LOG_TYPE {
    RB_LOG_DEBUG = 0x01,
    RB_LOG_WARN  = 0x02,
    RB_LOG_ERROR = 0x04,
};

#define RB_LOG_BUFSIZE      128 
#define RB_LOG_FILESIZE     (1024 * 1024 * 100)
#define RB_LOG_DIR_MODE     0777  
#define RB_LOG_FILE_OPEN_MODE (O_RDWR | O_APPEND | O_CREAT)
#define RB_LOG_FILE_MODE    0666
#define RB_LOG_TIME_FMTSTR_LEN 19

extern rb_log_t *_log;

void rb_log_init(void);
void rb_log_destroy(void);
void rb_log_output(int level, const char *_FILE, int _LINE,
        const char *fmt, ...);
void rb_log_set_mask(int level);
void rb_log_unset_mask(int level);
void rb_log_ban(void);
void rb_log_flush(rb_evloop_t *loop);

#define rb_log_debug(...) \
    rb_log_output(RB_LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define rb_log_warn(...) \
    rb_log_output(RB_LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define rb_log_error(...) \
    rb_log_output(RB_LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)

#endif /* _RIBEV_LOGGER_H */
