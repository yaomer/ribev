#ifndef _RIBEV_LOG_FMT_H
#define _RIBEV_LOG_FMT_H

void rb_log_creat_dir(void);
int rb_log_creat_file(void);
void rb_log_fmt(const char *level, const char *_FILE, int _LINE,
        const char *fmt, va_list ap);
const char *rb_logstr(int level);

#endif /* _RIBEV_LOG_FMT_H */
