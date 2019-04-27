#ifndef _RIBEV_UTIL_H
#define _RIBEV_UTIL_H

#include <inttypes.h>

long rb_thread_id(void);
int64_t rb_now(void);
char *rb_get_timestr(int64_t ms, char *buf, size_t len);
char *rb_local_timestr(char *buf, size_t len);

#endif /* _RIBEV_UTIL_H */
