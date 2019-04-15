#ifndef _RIBEV_TASK_H
#define _RIBEV_TASK_H

typedef struct rb_task {
    void (*callback)(void **);
    /* 为了传递可变参数表，避免在传递多个参数时
     * 定义不必要的结构体 
     */
    void **argv;
    /* free_argv是一个函数指针数组，用于释放对应的argv。
     * free_argv[i]应该设置为释放对应argv[i]的自定义函数；
     * 如果不设置则默认为NULL，这样销毁task就仅仅只是简单
     * 销毁argv[i]这个指针，这时如果argv[i]指向堆上的一块内存，
     * 就会造成内存泄漏。
     * 不需要对应free_argv[i]的一种常见情形是：我们保存了多个
     * 指向某块内存的指针，我们只有在彻底放弃使用这块内存中的数据
     * 时才需要释放它，而其他情形我们仅仅只需销毁保存的那个指针即可。
     * 否则稍不注意就会造成过早释放，导致我们原来的指针指向一块
     * 垃圾内存；或者造成重复释放，导致程序退出。
     */
    void (**free_argv)(void *);
} rb_task_t;

rb_task_t *rb_alloc_task(int argc);
void rb_free_task(void *arg);

#endif /* _RIBEV_TASK_H */
