#ifndef _RIBEV_EVLOOP_H
#define _RIBEV_EVLOOP_H

#include <pthread.h>
#include <stdatomic.h>
#include "fwd.h"
#include "hash.h"

typedef struct rb_evloop {
    /* evop指向一个[I/O Multiplexing]实例，这个值
     * 是在编译时决定的，所以它应该被声明为[void *]类型，
     * 以接受所有可能的实例类型 */
    void *evop;
    /* evsel指向一个函数指针数组，这个数组提供[evop]的
     * 具体操作 */
    const rb_evop_t *evsel;
    /* chlist保存所有注册的chl，并且chl的生命期只由chlist决定，
     * 其他对象可以共享chl，即又有一个指向chl的指针，但绝不能
     * 销毁chl */
    rb_hash_t *chlist;    
    rb_timer_t *timer;
    /* active_chls由evsel->dispatch()填充本次事件循环中
     * 活跃的chl */
    rb_queue_t *active_chls;
    /* 一个便于在多线程之间调度任务的任务队列。其他线程可以
     * 只管向qtask中添加task，然后io线程通过qtask来执行这些
     * task，从而避免一些复杂的线程同步问题 */
    rb_queue_t *qtask;
    pthread_mutex_t mutex;
    /* 由socketpair()创建，需要时用来唤醒阻塞在evsel->dispatch()
     * 上的io线程 */
    int wakefd[2];
    /* 标志是否退出loop */
    atomic_int quit;
    /* loop所在线程的tid */
    int tid;
} rb_evloop_t;

#define rb_search_chl(loop, fd) \
    ((rb_channel_t *)rb_hash_search((loop)->chlist, fd))

rb_evloop_t *rb_evloop_init(void);
void rb_evloop_run(rb_evloop_t *loop);
void rb_evloop_quit(rb_evloop_t *loop);

int rb_in_loop_thread(rb_evloop_t *loop);
void rb_wakeup(rb_evloop_t *loop);
void rb_wakeup_read(rb_channel_t *chl);
void rb_run_in_loop(rb_evloop_t *loop, rb_task_t *task);

#endif /* _RIBEV_EVLOOP_H */
