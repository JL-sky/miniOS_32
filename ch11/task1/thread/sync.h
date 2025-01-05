#ifndef __THREAD_SYNC_H
#define __THREAD_SYNC_H

#include "list.h"
#include "stdint.h"
#include "thread.h"

/*信号量结构体*/
struct semaphore
{
    // 信号量值
    uint8_t value;
    // 阻塞在当前信号量上的线程的阻塞队列
    struct list waiters;
};

/*锁结构*/
struct lock
{
    // 锁的持有者
    struct task_struct *holder;
    // 锁上的信号量
    struct semaphore semaphore;
    // 加锁者加锁的次数，也就是加锁者访问公共资源的次数
    uint32_t holder_repeat_nr;
};
void sema_init(struct semaphore *psema, uint8_t value);
void sema_down(struct semaphore *psema);
void sema_up(struct semaphore *psema);
void lock_init(struct lock *plock);
void lock_acquire(struct lock *plock);
void lock_release(struct lock *plock);

#endif