/*
 * 一个简单的线程池示例
 */

#include <stdio.h>
#include "alloc.h"
#include "threadpool.h"
#include "task.h"
#include "queue.h"
#include "lock.h"

static void *
rb_threadpool_func(void *arg)
{
    rb_threadpool_t *pool = (rb_threadpool_t *)arg;

    while (1) {
        rb_lock(&pool->mutex);
        while (!pool->quit && rb_queue_is_empty(pool->qtask))
            rb_wait(&pool->cond, &pool->mutex);
        if (pool->quit) {
            rb_unlock(&pool->mutex);
            break;
        }
        rb_task_t *t = (rb_task_t *)rb_queue_front(pool->qtask)->data;
        rb_queue_pop(pool->qtask);
        rb_unlock(&pool->mutex);
        t->callback(t->argv);
        rb_free_task(t);
    }
    return 0;
}

rb_threadpool_t *
rb_threadpool_init(int threads)
{
    rb_threadpool_t *pool = rb_malloc(sizeof(rb_threadpool_t));

    pool->qtask = rb_queue_init();
    rb_lock_init(&pool->mutex);
    rb_cond_init(&pool->cond);
    pool->tid = rb_calloc(threads, sizeof(pthread_t));
    pool->threads = threads;
    pool->quit = 0;
    for (int i = 0; i < threads; i++)
        pthread_create(&pool->tid[i], NULL, rb_threadpool_func, pool);

    return pool;
}

void
rb_threadpool_add(rb_threadpool_t *pool, rb_task_t *task)
{
    rb_lock(&pool->mutex);
    rb_queue_push(pool->qtask, task);
    rb_unlock(&pool->mutex);
    rb_notify(&pool->cond);
}

void
rb_threadpool_destroy(rb_threadpool_t **_pool)
{
    rb_threadpool_t *pool = *_pool;

    pool->quit = 1;
    rb_notify_all(&pool->cond);
    for (int i = 0; i < pool->threads; i++)
        pthread_join(pool->tid[i], NULL);
    rb_free(pool->tid);
    rb_queue_destroy(&pool->qtask);
    rb_lock_destroy(&pool->mutex);
    rb_cond_destroy(&pool->cond);
    rb_free(*_pool);
    *_pool = NULL;
}
