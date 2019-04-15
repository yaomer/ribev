#include "task.h"
#include "alloc.h"

rb_task_t *
rb_alloc_task(int argc)
{
    rb_task_t *t = rb_calloc(1, sizeof(rb_task_t));
    t->argv = rb_calloc(argc + 1, sizeof(void *));
    t->free_argv = rb_calloc(argc, sizeof(void *));
    return t;
}

void
rb_free_task(void *arg)
{
    rb_task_t *t = (rb_task_t *)arg;

    for (int i = 0; t->argv[i]; i++)
        if (t->free_argv[i])
            t->free_argv[i](t->argv[i]);
}
