#ifndef _RIBEV_LOCK_H
#define _RIBEV_LOCK_H

#include <pthread.h>

#define rb_creat_thread(tid, func, arg) pthread_create(tid, NULL, func, arg)

#define rb_lock_init(mutex) pthread_mutex_init(mutex, NULL)
#define rb_lock(mutex) pthread_mutex_lock(mutex)
#define rb_unlock(mutex) pthread_mutex_unlock(mutex)
#define rb_lock_destroy(mutex) pthread_mutex_destroy(mutex)

#define rb_cond_init(cond) pthread_cond_init(cond, NULL)
#define rb_wait(cond, mutex) pthread_cond_wait(cond, mutex)
#define rb_notify(cond) pthread_cond_signal(cond)
#define rb_notify_all(cond) pthread_cond_broadcast(cond)
#define rb_cond_destroy(cond) pthread_cond_destroy(cond)

#endif /* _RIBEV_LOCK_H */
