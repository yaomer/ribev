#ifndef _RIBEV_QUEUE_H
#define _RIBEV_QUEUE_H

struct queue_node {
    void *data;
    struct queue_node *next;
    struct queue_node *prev;
};

typedef struct rb_queue {
    struct queue_node *front;
    void (*free_data)(void *);
} rb_queue_t;

rb_queue_t *rb_queue_init(void);
void *rb_queue_front(rb_queue_t *q);
int rb_queue_is_empty(rb_queue_t *q);
void rb_queue_push(rb_queue_t *q, void *data);
void rb_queue_pop(rb_queue_t *q);
void rb_queue_del(rb_queue_t *q, struct queue_node *qs);
void rb_queue_destroy(rb_queue_t **_q);

#define rb_queue_set_free(q, f) (q->free_data = f)

#endif /* _RIBEV_QUEUE_H */
