#include <stdio.h>
#include <string.h>
#include "alloc.h"
#include "hash.h"

/*
 * hash的初始大小
 */
#define GV_HASH_INIT_SIZE 64
/*
 * 装载因子，我们在每次插入时都需要计算当前的装载因子lf，
 * 如果lf > GV_HASH_LOAD_FACTOR，就需要对hash进行扩张，
 * 从而保证高效的查找效率
 */
#define GV_HASH_LOAD_FACTOR 0.75

/*
 * 分配一个size大小的bucket
 */
static struct hash_node **
rb_alloc_buckets(size_t size)
{
    struct hash_node **_h = rb_malloc(size * sizeof(struct hash_node *));
    return _h;
}

/*
 * 分配一个hash_node节点
 */
static struct hash_node *
rb_alloc_hash_node(void)
{
    struct hash_node *h = rb_calloc(1, sizeof(struct hash_node));
    return h;
}

rb_hash_t *
rb_hash_init(void)
{
    rb_hash_t *h = rb_malloc(sizeof(rb_hash_t));

    h->hashsize = GV_HASH_INIT_SIZE;
    h->buckets = rb_alloc_buckets(h->hashsize);
    h->hashnums = 0;
    h->free_data = NULL;

    return h;
}

static unsigned
__hash(unsigned key)
{
    /* 扰乱key，增加产生hashval的随机性 */
    return key ^ (key >> 16);
}

static unsigned
rb_hash(rb_hash_t *h, unsigned key)
{
    /* 当h->hashsize = 2^n时，可以用[& (h->hashsize - 1)]
     * 代替[% h->hashsize]来提高效率
     */
    return __hash(key) & (h->hashsize - 1);
}

/*
 * 将np插入到指定桶的开头，这样可以做到O(1)时间插入。
 */
static void
__insert(rb_hash_t *h, struct hash_node *np)
{
    unsigned hashval = rb_hash(h, np->key);
    np->next = h->buckets[hashval];
    h->buckets[hashval] = np;
    h->hashnums++;
}

static double
rb_hash_lf(rb_hash_t *h)
{
    /* 计算装载因子 */
    return (h->hashnums * 1.0) / h->hashsize;
}

/*
 * 扩充hash。我们在这里采用最简单的方法：
 * 即将原hash中的所有key进行重新映射，然后再插入到新hash中。
 * 这样做的缺点就是可能会导致这次insert产生较大的延迟。
 * 一种改进是：使用线性散列技术。
 */
static void
rb_rehash(rb_hash_t *h)
{
    struct hash_node **b = h->buckets;
    unsigned bsize = h->hashsize;
    h->hashnums = 0;
    h->hashsize *= 2;
    h->buckets = rb_alloc_buckets(h->hashsize);

    for (int i = 0; i < bsize; i++) {
        struct hash_node *np = b[i];
        for ( ; np; np = np->next)
            __insert(h, np);
    }
    rb_free(b);
}

/*
 * 将<key, data>插入到hash中，这里对data仅执行浅拷贝。
 */
void
rb_hash_insert(rb_hash_t *h, unsigned key, void *data)
{
    struct hash_node *np = rb_alloc_hash_node();
    np->key = key;
    np->data = data;

    if (rb_hash_lf(h) >= GV_HASH_LOAD_FACTOR)
        rb_rehash(h);
    __insert(h, np);
}

void
rb_hash_delete(rb_hash_t *h, unsigned key)
{
    struct hash_node *p, *pre;
    unsigned hashval = rb_hash(h, key);

    p = h->buckets[hashval];
    for (pre = NULL; p; p = p->next) {
        if (p->key == key)
            break;
        pre = p;
    }
    if (pre)
        pre->next = p->next;
    else
        h->buckets[hashval] = p->next;
    if (h->free_data) {
        h->free_data(p->data);
        p->data = NULL;
    }
    rb_free(p);
}

/*
 * [rehash]保证了[search]的平均时间复杂度是O(1)
 */
void *
rb_hash_search(rb_hash_t *h, unsigned key)
{
    struct hash_node *np;

    for (np = h->buckets[rb_hash(h, key)]; np; np = np->next)
        if (np->key == key)
            return np->data;
    return NULL;
}

void
rb_hash_destroy(rb_hash_t **_h)
{
    rb_hash_t *h = *_h;

    for (int i = 0; i < h->hashsize; i++)
        if (h->buckets[i]) {
            struct hash_node *p = h->buckets[i];
            while (p) {
                struct hash_node *np = p->next;
                if (h->free_data)
                    h->free_data(p->data);
                rb_free(p);
                p = np;
            }
        }
    rb_free (*_h);
    *_h = NULL;
}
