#include "sync.h"
#include "global.h"
#include "debug.h"
#include "interrupt.h"
/*初始化信号量*/
void sema_init(struct semaphore *psema, uint8_t value)
{
    psema->value = value;
    // 初始化该信号量上的阻塞队列
    list_init(&psema->waiters);
}

/*信号量的P操作*/
void sema_down(struct semaphore *psema)
{
    // 对阻塞队列公共资源的访问需要关中断以避免访问过程中背打断
    enum intr_status old_status = intr_get_status();

    // 如果当前可用资源（信号量的值）为0，则应当阻塞当前线程
    while (psema->value == 0)
    {
        if (elem_find(&psema->waiters, &running_thread()->general_tag))
            PANIC("sema_down: thread blocked has been in waiters_list\n");
        list_append(&psema->waiters, &running_thread()->general_tag);
        thread_block(TASK_BLOCKED);
    }
    // 可以让当前线程访问公共资源，同时让可访问的资源数减去一
    psema->value--;
    ASSERT(psema->value == 0);
    // 恢复中断之前的状态
    intr_set_status(old_status);
}
/*信号量的V操作*/
void sema_up(struct semaphore *psema)
{
    enum intr_status old_status = intr_get_status();

    // 检查该信号量上的阻塞队列是否为空
    // 如果不空，则直接唤醒一个线程
    if (!list_empty(&psema->waiters))
    {
        struct task_struct *thread_blocked = elem2entry(struct task_struct, general_tag, list_pop(&psema->waiters));
        thread_unblock(thread_blocked);
    }
    psema->value++;
    ASSERT(psema->value == 1);

    intr_set_status(old_status);
}
/*初始化锁*/
void lock_init(struct lock *plock)
{
    plock->holder = NULL;
    sema_init(&plock->semaphore, 1);
    plock->holder_repeat_nr = 0;
}
/*获取锁*/
void lock_acquire(struct lock *plock)
{
    if (plock->holder != running_thread())
    {
        plock->holder = running_thread();
        sema_down(&plock->semaphore);
        ASSERT(plock->holder_repeat_nr == 0);
        plock->holder_repeat_nr = 1;
    }
    else
    {
        plock->holder_repeat_nr++;
    }
}
/*释放锁*/
void lock_release(struct lock *plock)
{
    ASSERT(plock->holder == running_thread());
    if (plock->holder_repeat_nr > 1)
    {
        plock->holder_repeat_nr--;
        return;
    }
    ASSERT(plock->holder_repeat_nr == 1);
    plock->holder_repeat_nr = 0;
    sema_up(&plock->semaphore);
    plock->holder = NULL;
}
