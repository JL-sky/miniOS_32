#include "print.h"
#include "init.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"
void thread_work_a(void *arg);
void thread_work_b(void *arg);

#if 0
int main(void)
{
    put_str("I am kernel\n");
    init_all();

    thread_start("thread_work_a", 31, thread_work_a, "pthread_create_A\n");
    thread_start("thread_work_b", 8, thread_work_b, "pthread_create_B\n");

    /*打开中断，主要是打开时钟中断，以让时间片轮转调度生效*/
    intr_enable();
    while (1)
    {
        intr_disable();
        put_str("Main");
        intr_disable();
    }
    return 0;
}

/* 线程执行函数 */
void thread_work_a(void *arg)
{
    char *para = (char *)arg;
    while (1)
    {
        intr_disable();
        put_str(para);
        intr_enable();
    }
}
/* 线程执行函数 */
void thread_work_b(void *arg)
{
    char *para = (char *)arg;
    while (1)
    {
        intr_disable();
        put_str(para);
        intr_enable();
    }
}
#endif
int main(void)
{
    put_str("I am kernel\n");
    init_all();

    thread_start("thread_work_a", 31, thread_work_a, "pthread_A ");
    thread_start("thread_work_b", 8, thread_work_b, "pthread_B ");

    /*打开中断，主要是打开时钟中断，以让时间片轮转调度生效*/
    intr_enable();
    while (1)
    {
        console_put_str("Main ");
    }
    return 0;
}

/* 线程执行函数 */
void thread_work_a(void *arg)
{
    char *para = (char *)arg;
    while (1)
    {
        console_put_str(para);
    }
}
/* 线程执行函数 */
void thread_work_b(void *arg)
{
    char *para = (char *)arg;
    while (1)
    {
        console_put_str(para);
    }
}