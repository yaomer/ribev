#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "alloc.h"
#include "evop.h"
#include "evloop.h"
#include "queue.h"
#include "vector.h"
#include "timer.h"
#include "channel.h"
#include "net.h"
#include "lock.h"
#include "task.h"
#include "log.h"

/*
 * 确保每个线程至多只能运行一个loop
 */
__thread rb_evloop_t *__loop = NULL;

rb_evloop_t *
rb_evloop_init(void)
{
    rb_evloop_t *loop = rb_malloc(sizeof(rb_evloop_t));
    if (__loop)
        rb_log_error("more than one loop in thread");
    else
        __loop = loop;

    /* 选择合适的I/O多路复用机制 */
    if (!(loop->evsel = evops[0]))
        rb_log_error("no supported I/O multiplexing");

    loop->evop = loop->evsel->init();
    loop->chlist = rb_hash_init();
    rb_hash_set_free(loop->chlist, rb_free_chl);
    loop->timer = rb_timer_init();
    loop->active_chls = rb_queue_init();
    loop->ready_chls = rb_queue_init();
    loop->qtask = rb_queue_init();
    rb_lock_init(&loop->mutex);
    rb_socketpair(loop->wakefd);
    loop->tid = rb_thread_id();
    loop->quit = 0;

    return loop;
}

/*
 * 判断当前线程是否是io线程
 */
int
rb_in_loop_thread(rb_evloop_t *loop)
{
    return loop->tid == rb_thread_id();
}

/*
 * 对于loop->wakefd，因为它是由socketpair()创建的，所以可以
 * 从wakefd[1]写，wakefd[0]读；也可以从wakefd[0]写，wakefd[1]读。
 * 只需保证不从同一端读写即可(否则会阻塞)。这一点有别于unix传统的pipe。
 * 不过我习惯上从wakefd[1]写，wakefd[0]读。
 */

void
rb_wakeup(rb_evloop_t *loop)
{
    uint64_t one = 1;
    ssize_t n = write(loop->wakefd[1], &one, sizeof(one));

    if (n != sizeof(one))
        rb_log_error("writes %zd bytes instead of 8", n);
}

void
rb_wakeup_read(rb_channel_t *chl)
{
    uint64_t one = 1;
    ssize_t n = read(chl->ev.ident, &one, sizeof(one));

    if (n != sizeof(one))
        rb_log_error("reads %zd bytes instead of 8", n);
}

static void
rb_wakeup_add(rb_evloop_t *loop)
{
    rb_channel_t *chl = rb_chl_init(loop);

    chl->ev.ident = loop->wakefd[0];
    rb_chl_set_cb(chl, rb_handle_event, rb_wakeup_read, NULL, NULL);
    rb_chl_add(chl);
    rb_chl_enable_read(chl);
}

/*
 * 利用这个函数我们可以很方便的在不同线程之间调配任务，
 * 它将所有的task(io线程或其他线程)都放到io线程中来执行。这样可以避免
 * 多个线程同时操纵一些共享资源(如socket fd)而产生的锁争用问题，
 * 也可以避免复杂的线程同步问题。
 */
void
rb_run_in_loop(rb_evloop_t *loop, rb_task_t *task)
{
    if (rb_in_loop_thread(loop))
        task->callback(task->argv);
    else {
        rb_lock(&loop->mutex);
        rb_queue_push(loop->qtask, task);
        rb_wakeup(loop);
        rb_unlock(&loop->mutex);
    }
}

/*
 * 一次事件循环中执行的最大任务数，避免长时间阻塞io。
 */
#define RB_RUN_MAX_TASKS 16

/*
 * 执行除io和timer之外的其他任务
 */
static void
rb_run_task(rb_evloop_t *loop)
{
    rb_lock(&loop->mutex);
    int ntasks = 0;
    rb_queue_t *tk = rb_queue_init();
    rb_queue_set_free(tk, rb_free_task);
    /* 将待执行的task先转移到tk中，以减小临界区的大小 */
    while (!rb_queue_is_empty(loop->qtask) && ntasks <= RB_RUN_MAX_TASKS) {
        rb_task_t *t = rb_queue_front(loop->qtask);
        rb_queue_push(tk, t);
        rb_queue_pop(loop->qtask);
        ntasks++;
    }
    rb_unlock(&loop->mutex);

    rb_log_debug("executes %d tasks", ntasks);
    while (ntasks > 0) {
        rb_task_t *t = rb_queue_front(tk);
        t->callback(t->argv);
        rb_queue_pop(tk);
        ntasks--;
    }
    rb_queue_destroy(&tk);
}

/*
 * 处理所有到来的io事件
 */
static void
rb_run_io(rb_evloop_t *loop)
{
    while (!rb_queue_is_empty(loop->active_chls)) {
        rb_channel_t *chl = rb_queue_front(loop->active_chls);
        if (chl->eventcb)
            chl->eventcb(chl);
        rb_queue_pop(loop->active_chls);
    }
}

/*
 * 将就绪连接加入到[loop]中
 */
static void
rb_add_ready_chls(rb_evloop_t *loop)
{
    rb_lock(&loop->mutex);
    while (!rb_queue_is_empty(loop->ready_chls)) {
        rb_channel_t *chl = rb_queue_front(loop->ready_chls);
        rb_chl_add(chl);
        rb_queue_pop(loop->ready_chls);
    }
    rb_unlock(&loop->mutex);
}

static void
rb_evloop_destroy(rb_evloop_t **_loop)
{
    rb_evloop_t *loop = *_loop;
    loop->evsel->dealloc(loop->evop);
    rb_hash_destroy(&loop->chlist);
    rb_timer_destroy(&loop->timer);
    rb_queue_destroy(&loop->active_chls);
    rb_queue_destroy(&loop->ready_chls);
    rb_queue_destroy(&loop->qtask);
    rb_lock_destroy(&loop->mutex);
    close(loop->wakefd[1]);
}

void
rb_evloop_run(rb_evloop_t *loop)
{
    rb_wakeup_add(loop);

    while (!loop->quit) {
        rb_add_ready_chls(loop);

        int timeout = rb_timer_out(loop->timer);
        int nevents = loop->evsel->dispatch(loop, loop->evop, timeout);

        if (nevents == 0) {
            rb_timer_tick(loop);
        } else if (nevents > 0) {
            rb_run_io(loop);
        } else {
            if (errno == EINTR
             || errno == EWOULDBLOCK
             || errno == EPROTO
             || errno == ECONNABORTED)
                rb_log_warn("dispatch");
            else
                rb_log_error("dispatch");
        }
        rb_run_task(loop);
    }
    rb_evloop_destroy(&loop);
}

void
rb_evloop_quit(rb_evloop_t *loop)
{
    loop->quit = 1;
}
