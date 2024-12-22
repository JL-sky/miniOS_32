#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"

#define PG_SIZE 4096

static void kernel_thread(thread_func *function, void *func_args)
{
    function(func_args);
}

/*初始化PCB*/
void init_thread(struct task_struct *pthread, char *name, int prio)
{
    memset(pthread, 0, sizeof(*pthread));
    strcpy(pthread->name, name);
    pthread->status = TASK_RUNNGING;
    pthread->priority = prio;
    /*
    一个线程的栈空间分配一页空间，将PCB放置在栈底
    pthread是申请的一页空间的起始地址，因此加上一页的大小，就是栈顶指针
    */
    pthread->self_kstack = (uint32_t *)((uint32_t)pthread + PG_SIZE);
    /*PCB的边界标记，防止栈顶指针覆盖掉PCB的内容*/
    pthread->stack_magic = 0x19991030;
}
/*根据PCB信息，初始化线程栈的运行信息*/
void thread_create(struct task_struct *pthread, thread_func function, void *func_args)
{
    /*给线程栈空间的顶部预留出中断栈信息的空间*/
    pthread->self_kstack = (uint32_t *)((int)(pthread->self_kstack) - sizeof(struct intr_stack));
    /*给线程栈空间的顶部预留出线程栈信息的空间*/
    pthread->self_kstack = (uint32_t *)((int)(pthread->self_kstack) - sizeof(struct thread_stack));
    // 初始化线程栈，保存线程运行时需要的信息
    struct thread_stack *kthread_stack = (struct thread_stack *)pthread->self_kstack;

    // 线程执行函数
    kthread_stack->eip = kernel_thread;
    kthread_stack->function = function;
    kthread_stack->func_args = func_args;
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->edi = kthread_stack->esi = 0;
}

/*根据线程栈的运行信息开始运行线程函数*/
struct task_struct *thread_start(char *name, int prio, thread_func function, void *func_args)
{
    /*1.分配一页的空间给线程作为线程执行的栈空间*/
    struct task_struct *thread = get_kernel_pages(1);
    /*2.初始化PCB，PCB里存放了线程的基本信息以及线程栈的栈顶指针*/
    init_thread(thread, name, prio);
    /*
    3.根据线程栈的栈顶指针，初始化线程栈，也就是初始化线程的运行信息
    比如线程要执行的函数，以及函数参数
    */
    thread_create(thread, function, func_args);
    /*4.上述准备好线程运行时的栈信息后，即可运行执行函数了*/
    asm volatile("movl %0,%%esp;    \
                pop %%ebp;          \
                pop %%ebx;          \
                pop %%edi;          \
                pop %%esi;          \
                ret"
                 :
                 : "g"(thread->self_kstack)
                 : "memory");
    return thread;
}