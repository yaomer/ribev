#ifndef _RIBEV_USER_H
#define _RIBEV_USER_H

#include <stddef.h>

/*
 * 供用户自定义的数据使用
 */
typedef struct rb_user {
    void *data;
    void *(*init)(void);
    void (*dealloc)(void *);
} rb_user_t;

#endif /* _RIBEV_USER_H */
