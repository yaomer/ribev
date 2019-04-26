/*
 * 一个简单的动态数组，实现了类似[C++ vector]中的一些功能
 */

#include <stdio.h>
#include <string.h>
#include "alloc.h"
#include "vector.h"

/*
 * [void *]移动的步长
 */
#define _STEP(v, x) ((x) * (v)->typesize)

/*
 * 访问data中的任一元素，不进行边界检查。
 */
void *
rb_vector_entry(rb_vector_t *v, size_t index)
{
    return v->data + _STEP(v, index);
}

rb_vector_t *
rb_vector_init(size_t typesize)
{
    rb_vector_t *v = rb_malloc(sizeof(rb_vector_t));

    v->size = 0;
    v->max_size = 2;
    v->typesize = typesize;
    v->data = rb_calloc(v->max_size, typesize);
    v->cfree = rb_calloc(v->max_size, 1);
    v->free_data = NULL;

    return v;
}

/*
 * 释放原有vector中可能需要释放的数据(data[0 ~ size - 1])，
 * 这通过调用用户注册的free_data()函数来实现。
 */
static void
rb_vector_free(rb_vector_t *v, void *data, size_t size)
{
    if (v->free_data)
        for (int i = 0; i < size && v->cfree[i]; i++)
            v->free_data(data + _STEP(v, i));
    rb_free(data);
}

/*
 * 扩充vector，扩充后的大小至少为[v->size + len]。
 * 我们先分配新的足够大的内存，然后再将原来的数据拷贝到新的内存地址中，
 * 最后再释放掉原来的内存空间。
 */
static void
rb_realloc_vector(rb_vector_t *v, size_t len)
{
    void *tv = v->data;
    char *tc = v->cfree;
    while (v->max_size < v->size + len)
        v->max_size *= 2;
    v->data = rb_calloc(v->max_size, v->typesize);
    v->cfree = rb_calloc(v->max_size, 1);
    if (tv) {
        memcpy(v->data, tv, _STEP(v, v->size));
        rb_free(tv);
    }
    if (tc) {
        memcpy(v->cfree, tc, v->size);
        rb_free(tc);
    }
}

/*
 * 仅将data指向的那块内存中的数据拷贝到vector中
 * struct A {
 *     int a;
 *     char *s;  // s = "hello, world!"
 * }; struct A *pA;
 * 如上，我们只能拷贝pA->s本身，一个8bytes的指针，不会也无法
 * 拷贝pA->s指向的内存
 */
void
rb_vector_push(rb_vector_t *v, void *data)
{
    if (v->size + 1 > v->max_size)
        rb_realloc_vector(v, 1);
    memcpy(v->data + _STEP(v, v->size), data, v->typesize);
    v->cfree[v->size++] = 1;
}

/*
 * 针对存储char类型做优化，避免多次调用rb_vector_push()
 */
void
rb_vector_push_str(rb_vector_t *v, const char *s, size_t len)
{
    if (v->max_size < v->size + len)
        rb_realloc_vector(v, len);
    memcpy(v->data + _STEP(v, v->size), s, _STEP(v, len));
    v->size += len;
}

/*
 * 弹出v->data[v->size - 1]，但它本身并不会马上释放，而是当再次
 * 扩充vector或销毁vector时才会释放。
 */
void
rb_vector_pop(rb_vector_t *v)
{
    v->size--;
    if (v->free_data && v->cfree[v->size])
        v->free_data(v->data + _STEP(v, v->size));
}

/*
 * 更改v->size的大小，在vector前面留一个空洞。
 */
void
rb_vector_resize(rb_vector_t *v, size_t size)
{
    if (size > v->max_size)
        rb_realloc_vector(v, size - v->size);
    v->size = size;
}

/*
 * 在vector后面预分配一块至少为size大小内存
 */
void
rb_vector_reserve(rb_vector_t *v, size_t size)
{
    if (size > v->max_size - v->size)
        rb_realloc_vector(v, size - v->size);
}

/*
 * 交换v->data[i]和v->data[j]
 */
void
rb_vector_swap(rb_vector_t *v, int i, int j)
{
    void *tmp = rb_malloc(v->typesize);

    memcpy(tmp, v->data + _STEP(v, i), v->typesize);
    memcpy(v->data + _STEP(v, i), v->data + _STEP(v, j), v->typesize);
    memcpy(v->data + _STEP(v, j), tmp, v->typesize);
    char t = v->cfree[i];
    v->cfree[i] = v->cfree[j];
    v->cfree[j] = t;
    rb_free(tmp);
}

void
rb_vector_destroy(rb_vector_t **_v)
{
    rb_vector_t *v = *_v;

    rb_vector_free(v, v->data, v->size);
    rb_free(*_v);
    *_v = NULL;
}
