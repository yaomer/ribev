/*
 * 这是一个双端循环队列
 */

#include <stdio.h>
#include <string.h>
#include "alloc.h"
#include "queue.h"

rb_queue_t *
rb_queue_init(void)
{
    rb_queue_t *q = rb_malloc(sizeof(rb_queue_t));

    q->front = rb_malloc(sizeof(struct queue_node));
    q->front->prev = q->front->next = q->front;
    q->free_data = NULL;

    return q;
}

/*
 * 返回队头节点
 */
struct queue_node *
rb_queue_front(rb_queue_t *q)
{
    return q->front->next;
}

int
rb_queue_is_empty(rb_queue_t *q)
{
    return q->front->prev == q->front;
}

/*
 * 入队操作
 */
#include "channel.h"
void
rb_queue_push(rb_queue_t *q, void *data)
{
    struct queue_node *qs = rb_malloc(sizeof(struct queue_node));
    qs->data = data;
    qs->next = qs->prev = NULL;

    if (rb_queue_is_empty(q)) {
        q->front->prev = qs;
        qs->next = q->front;
        q->front->next = qs;
        qs->prev = q->front;
    } else {
        qs->prev = q->front->prev;
        q->front->prev->next = qs;
        q->front->prev = qs;
        qs->next = q->front;
    }
}

/*
 * 出队操作
 */
void
rb_queue_pop(rb_queue_t *q)
{
    rb_queue_del(q, q->front->next);
}

/*
 * O(1)时间删除任一节点qs
 */
void
rb_queue_del(rb_queue_t *q, struct queue_node *qs)
{
    if (rb_queue_is_empty(q))
        return;

    qs->prev->next = qs->next;
    qs->next->prev = qs->prev;
    if (q->free_data) {
        q->free_data(qs->data);
        qs->data = NULL;
    }
    rb_free(qs);
}

void
rb_queue_destroy(rb_queue_t **_q)
{
    rb_queue_t *q = *_q;

    while (!rb_queue_is_empty(q))
        rb_queue_pop(q);
    rb_free(*_q);
    *_q = NULL;
}
