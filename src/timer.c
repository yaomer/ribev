#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "timer.h"
#include "vector.h"
#include "evloop.h"
#include "task.h"
#include "alloc.h"

#define parent(i)   (i / 2)
#define left(i)     (i * 2)
#define right(i)    (i * 2 + 1)

#define timeout(t, i) (((rb_timestamp_t *)rb_vector_entry(t, i))->timeout)

/*
 * 返回当前日历时间的时间戳(ms)
 */
int64_t
rb_now(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

rb_timestamp_t *
rb_alloc_timestamp(void)
{
    rb_timestamp_t *t = rb_calloc(1, sizeof(rb_timestamp_t));
    return t;
}

void
rb_free_timestamp(void *arg)
{
    rb_timestamp_t *t = (rb_timestamp_t *)arg;
    rb_free_task(t->task);
    rb_free(t);
}

rb_timer_t *
rb_timer_init(void)
{
    rb_timer_t *t = rb_malloc(sizeof(rb_timer_t));

    t->timer = rb_vector_init(sizeof(rb_timestamp_t));
    /* 让heap数组下标从1开始 */
    rb_vector_resize(t->timer, 1);

    return t;
}

/*
 * 维护最小堆的性质
 */
static void
min_heap(rb_timer_t *t, int i)
{
    int l, r, smallest;
    size_t size = rb_vector_size(t->timer);

    l = left(i);
    r = right(i);

    if (l > size)
        return;
    if (timeout(t->timer, l) < timeout(t->timer, i))
        smallest = l;
    else
        smallest = i;
    if (r > size)
        goto next;
    if (timeout(t->timer, r) < timeout(t->timer, smallest))
        smallest = r;
next:
    if (smallest != i) {
        rb_vector_swap(t->timer, i, smallest);
        min_heap(t, smallest);
    }
}

/*
 * 返回堆顶元素
 */
static rb_timestamp_t *
rb_timer_top(rb_timer_t *t)
{
    if (rb_vector_size(t->timer) < 2)
        return NULL;
    else
        return (rb_timestamp_t *)rb_vector_entry(t->timer, 1);
}

/*
 * 获取堆顶元素的超时值
 */
int
rb_timer_out(rb_timer_t *t)
{
    rb_timestamp_t *tm = rb_timer_top(t);
    return tm ? llabs(tm->timeout - rb_now()) : -1;
}

/*
 * 添加一个定时事件
 */
static void
rb_timer_add(rb_timer_t *t, rb_timestamp_t *tm)
{
    int i, j;

    tm->timeout += rb_now();

    rb_vector_push(t->timer, tm);
    i = rb_vector_size(t->timer) - 1;
    j = parent(i);
    while (i > 1 && timeout(t->timer, i) < timeout(t->timer, j)) {
        rb_vector_swap(t->timer, i, j);
        i = j;
        j = parent(i);
    }
}

/*
 * 弹出堆顶元素，即最小超时事件
 */
static void
rb_timer_del(rb_timer_t *t)
{
    size_t size = rb_vector_size(t->timer) - 1;

    if (size < 1) {
        return;
    }

    rb_vector_swap(t->timer, 1, size);
    rb_vector_pop(t->timer);
    min_heap(t, 1);
}

/*
 * 处理所有定时事件
 */
void
rb_timer_tick(rb_evloop_t *loop)
{
    rb_timestamp_t *tm = rb_timer_top(loop->timer);
    int64_t timeout = tm->timeout;

    /* 因为我们总是在io线程中添加timer，所以处理timer时
     * 不需要加锁
     */
    while ((tm = rb_timer_top(loop->timer))) {
        if (tm->timeout == timeout) {
            tm->task->callback(tm->task->argv);
            rb_timer_del(loop->timer);
            if (tm->interval > 0) {
                tm->timeout = tm->interval;
                rb_timer_add(loop->timer, tm);
            }
        } else
            break;
    }
}

static void
__rb_timer_add(void **argv)
{
    rb_timer_t *t = (rb_timer_t *)argv[0];
    rb_timestamp_t *tm = (rb_timestamp_t *)argv[1];
    rb_timer_add(t, tm);
}

/*
 * 在io线程添加timer
 */
static void
__rb_timer_run(rb_evloop_t *loop, rb_timestamp_t *tm)
{
    rb_task_t *t = rb_alloc_task(2);
    t->callback = __rb_timer_add;
    t->argv[0] = loop->timer;
    t->argv[1] = tm;
    t->free_argv[1] = rb_free_timestamp;
    rb_run_in_loop(loop, t);
}

/*
 * 在将来某个时间执行一次task
 */
void
rb_run_at(rb_evloop_t *loop, int64_t timeval, rb_task_t *t)
{
    if (timeval < rb_now())
        return;

    rb_timestamp_t *tm = rb_alloc_timestamp();
    tm->timeout = timeval - rb_now();
    tm->interval = 0;
    tm->task = t;
    __rb_timer_run(loop, tm);
}

/*
 * 在timeout(ms)后执行一次task
 */
void
rb_run_after(rb_evloop_t *loop, int64_t timeout, rb_task_t *t)
{
    rb_timestamp_t *tm = rb_alloc_timestamp();
    tm->timeout = timeout;
    tm->interval = 0;
    tm->task = t;
    __rb_timer_run(loop, tm);
}

/*
 * 每隔timeout(ms)执行一次task
 */
void
rb_run_every(rb_evloop_t *loop, int64_t interval, rb_task_t *t)
{
    rb_timestamp_t *tm = rb_alloc_timestamp();
    tm->timeout = tm->interval = interval;
    tm->task = t;
    __rb_timer_run(loop, tm);
}