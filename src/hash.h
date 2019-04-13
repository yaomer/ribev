#ifndef _RIBEV_HASH_H
#define _RIBEV_HASH_H

#include <stddef.h>

struct hash_node {
    unsigned key;
    void *data;
    struct hash_node *next;
};

typedef struct rb_hash {
    struct hash_node **buckets;
    size_t hashsize;
    size_t hashnums;  /* 当前已插入到hash中的元素数量 */
    void (*free_data)(void *);
} rb_hash_t;

rb_hash_t *rb_hash_init(void);
void rb_hash_insert(rb_hash_t *h, unsigned key, void *data);
void rb_hash_delete(rb_hash_t *h, unsigned key);
void *rb_hash_search(rb_hash_t *h, unsigned key);
void rb_hash_destroy(rb_hash_t **_h);

#define rb_hash_set_free(h, f) (h->free_data = f)

#endif /* _RIBEV_HASH_H */
