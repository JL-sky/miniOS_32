#include "print.h"
#include "init.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"

/* 临时为测试添加 */
#include "ioqueue.h"
#include "keyboard.h"
void thread_work_a(void *arg);
void thread_work_b(void *arg);

int main(void)
{
    put_str("I am kernel\n");
    init_all();

    thread_start("consumer_a", 31, thread_work_a, "consumer_A:");
    thread_start("consumer_b", 8, thread_work_b, "consumer_B:");

    /*打开中断，主要是打开时钟中断，以让时间片轮转调度生效*/
    intr_enable();
    while (1)
        ;
    return 0;
}

/* 线程执行函数 */
void thread_work_a(void *arg)
{
    char *para = (char *)arg;
    while (1)
    {
        enum intr_status old_status = intr_disable();
        if (!ioq_empty(&kbd_buf))
        {
            console_put_str(para);
            char byte = ioq_getchar(&kbd_buf);
            console_put_char(byte);
            console_put_str("   ");
        }
        intr_set_status(old_status);
    }
}
/* 线程执行函数 */
void thread_work_b(void *arg)
{
    char *para = (char *)arg;
    while (1)
    {
        enum intr_status old_status = intr_disable();
        if (!ioq_empty(&kbd_buf))
        {
            console_put_str(para);
            char byte = ioq_getchar(&kbd_buf);
            console_put_char(byte);
            console_put_str("   ");
        }
        intr_set_status(old_status);
    }
}