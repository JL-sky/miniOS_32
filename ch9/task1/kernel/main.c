#include "print.h"
#include "init.h"
#include "thread.h"
void thread_work(void *arg);

int main(void)
{
    put_str("I am kernel\n");
    init_all();

    thread_start("thread_work", 31, thread_work, "pthread_create\n");

    while (1)
        ;
    return 0;
}

/* 线程执行函数 */
void thread_work(void *arg)
{
    char *para = (char *)arg;

    int i = 10;
    while (i--)
        put_str(para);
}
