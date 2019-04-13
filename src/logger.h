#ifndef _RIBEV_LOGGER_H
#define _RIBEV_LOGGER_H

#include <pthread.h>
#include "fwd.h"

typedef struct rb_log {
    rb_buffer_t *buf;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_t tid;
    int quit;
    int mask;
} rb_log_t;

#define RB_LOG_BUFSIZE      128 
#define RB_LOG_FILESIZE     (1024 * 1024 * 100)
#define RB_LOG_DIR_MODE     0777  
#define RB_LOG_FILE_OPEN_MODE (O_RDWR | O_APPEND | O_CREAT)
#define RB_LOG_FILE_MODE    0666
#define RB_LOG_TIME_FMTSTR_LEN 19

extern Log *_log;

void rb_log_init(void);
void rb_log_destroy(void);
void rb_log_debug(const char *_FILE, int _LINE, char *fmt, ...);
void rb_log_warn(const char *_FILE, int _LINE, char *fmt, ...);
void rb_log_error(const char *_FILE, int _LINE, char *fmt, ...);
void rb_log_set_mask(int level);
void rb_log_unset_mask(int level);
void rb_log_ban(void);
void rb_log_flush(Evloop *loop);

#define RB_LOG_DEBUG(...) rb_log_debug(__FILE__, __LINE__, __VA_ARGS__) 
#define RB_LOG_WARN(...) rb_log_warn(__FILE__, __LINE__, __VA_ARGS__)
#define RB_LOG_ERROR(...) rb_log_error(__FILE__, __LINE__, __VA_ARGS__)

#endif /* _RIBEV_LOGGER_H */
