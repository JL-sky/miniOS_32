#include "sync.h"
#include "list.h"
#include "global.h"
#include "debug.h"
#include "interrupt.h"

// 用于初始化信号量，传入参数就是指向信号量的指针与初值
void sema_init(struct semaphore *psema, uint8_t value)
{
    psema->value = value;       // 为信号量赋初值
    list_init(&psema->waiters); // 初始化信号量的等待队列
}

/*初始化锁*/
void lock_init(struct lock *plock)
{
    plock->holder = NULL;
    sema_init(&plock->semaphore, 1);
    plock->holder_repeat_nr = 0;
}

/*信号量的P操作*/
void sema_down(struct semaphore *psema)
{
    // 对阻塞队列公共资源的访问需要关中断以避免访问过程中背打断
    enum intr_status old_status = intr_disable();

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

// 信号量的up操作，也就是+1操作，传入参数是指向要操作的信号量的指针。且释放信号量时，应唤醒阻塞在该信号量阻塞队列上的一个进程
void sema_up(struct semaphore *psema)
{
    /* 关中断,保证原子操作 */
    enum intr_status old_status = intr_disable();
    ASSERT(psema->value == 0);
    if (!list_empty(&psema->waiters))
    { // 判断信号量阻塞队列应为非空，这样才能执行唤醒操作
        struct task_struct *thread_blocked = elem2entry(struct task_struct, general_tag, list_pop(&psema->waiters));
        thread_unblock(thread_blocked);
    }
    psema->value++;
    ASSERT(psema->value == 1);
    /* 恢复之前的中断状态 */
    intr_set_status(old_status);
}

// 获取锁的函数,传入参数是指向锁的指针
void lock_acquire(struct lock *plock)
{
    // 这是为了排除掉线程自己已经拿到了锁，但是还没有释放就重新申请的情况
    if (plock->holder != running_thread())
    {
        sema_down(&plock->semaphore); // 对信号量进行down操作
        plock->holder = running_thread();
        ASSERT(plock->holder_repeat_nr == 0);
        plock->holder_repeat_nr = 1; // 申请了一次锁
    }
    else
    {
        plock->holder_repeat_nr++;
    }
}

// 释放锁的函数，参数是指向锁的指针
void lock_release(struct lock *plock)
{
    ASSERT(plock->holder == running_thread());
    // 如果>1，说明自己多次申请了该锁，现在还不能立即释放锁
    if (plock->holder_repeat_nr > 1)
    {
        plock->holder_repeat_nr--;
        return;
    }
    ASSERT(plock->holder_repeat_nr == 1); // 判断现在lock的重复持有数是不是1只有为1，才能释放

    plock->holder = NULL; // 这句必须放在up操作前，因为现在并不在关中断下运行，有可能会被切换出去，如果在up后面，就可能出现还没有置空，
                          // 就切换出去，此时有了信号量，下个进程申请到了，将holder改成下个进程，这个进程切换回来就把holder改成空，就错了
    plock->holder_repeat_nr = 0;
    sema_up(&plock->semaphore); // 信号量的V操作,也是原子操作
}
