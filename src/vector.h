#ifndef _RIBEV_VECTOR_H
#define _RIBEV_VECTOR_H

#include <stddef.h>

typedef struct rb_vector {
    void *data;   /* 一个可变数组 */
    size_t size;  /* 已使用大小 */
    size_t max_size;  /* 最大大小 */
    size_t typesize;  /* 存储的类型大小 */
    /* 当data[i]中存有指向heap的指针时，用户需要提供free_data()
     * 函数用来释放其内存，但用户切记不能释放&data[i]本身，因为
     * 它不一定是由[*alloc()]返回的指针，我们的分配策略是每次分配
     * 一大块内存，因而当释放时，只需调用一次rb_free(data)即可，
     * 不能对每个&data[i]都调用rb_free()。
     */
    void (*free_data)(void *);
} rb_vector_t;

#define rb_vector_size(v) ((v)->size)
#define rb_vector_max_size(v) ((v)->max_size)

void *rb_vector_entry(rb_vector_t *v, size_t index);
rb_vector_t *rb_vector_init(size_t typesize);
void rb_vector_push(rb_vector_t *v, void *data);
void rb_vector_push_str(rb_vector_t *v, const char *s, size_t len);
void rb_vector_pop(rb_vector_t *v);
void rb_vector_resize(rb_vector_t *v, size_t size);
void rb_vector_reserve(rb_vector_t *v, size_t size);
void rb_vector_swap(rb_vector_t *v, int i, int j);
void rb_vector_destroy(rb_vector_t **_v);

#define rb_vector_set_free(v, f) ((v)->free_data = f)

#endif /* _RIBEV_VECTOR_H */
