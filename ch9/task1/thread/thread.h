#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H

#include "stdint.h"
/*定义执行函数*/
typedef void thread_func(void *);

/*定义进程或者线程的任务状态*/
enum task_status
{
    TASK_RUNNGING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED,
};

/*中断发生时调用中断处理程序的压栈情况*/
struct intr_stack
{
    uint32_t vec_no;
    // pushad的压栈情况
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    // 中断调用时处理器自动压栈的情况
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    uint32_t err_code;
    void (*eip)(void);
    uint32_t cs;
    uint32_t eflags;
    void *esp;
    uint32_t ss;
};

/*定义线程栈，存储线程执行时的运行信息*/
struct thread_stack
{
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;

    // 一个函数指针，指向线程执行函数，目的是为了实现通用的线程函数调用
    void (*eip)(thread_func *func, void *func_args);
    // 以下三条是模仿call进入thread_start执行的栈内布局构建的，call进入就会压入参数与返回地址，因为我们是ret进入kernel_thread执行的
    // 要想让kernel_thread正常执行，就必须人为给它造返回地址，参数
    void(*unused_retaddr); // 一个栈结构占位
    thread_func *function;
    void *func_args;
};

/*PCB结构体*/
struct task_struct
{
    // 线程栈的栈顶指针
    uint32_t *self_kstack;
    // 线程状态
    enum task_status status;
    // 线程的优先级
    uint8_t priority;
    // 线程函数名
    char name[16];
    // 用于PCB结构体的边界标记
    uint32_t stack_magic;
};

/*初始化PCB*/
void init_thread(struct task_struct *pthread, char *name, int prio);
/*根据PCB信息，初始化线程栈的运行信息*/
void thread_create(struct task_struct *pthread, thread_func function, void *func_args);
/*根据线程栈的运行信息开始运行线程函数*/
struct task_struct *thread_start(char *name, int prio, thread_func function, void *func_args);

#endif