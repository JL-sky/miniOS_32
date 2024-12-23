#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H

#include "stdint.h"
#include "list.h"
#include "interrupt.h"
#include "debug.h"
/*定义执行函数*/
typedef void thread_func(void *);

/*定义进程或者线程的任务状态*/
enum task_status
{
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED,
};

/*中断发生时调用中断处理程序的压栈情况*/
/***********   中断栈intr_stack   ***********
 * 此结构用于中断发生时保护程序(线程或进程)的上下文环境:
 * 进程或线程被外部中断或软中断打断时,会按照此结构压入上下文
 * 寄存器,  intr_exit中的出栈操作是此结构的逆操作
 * 此栈在线程自己的内核栈中位置固定,所在页的最顶端
 ********************************************/
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
/***********  线程栈thread_stack  ***********
 * 线程自己的栈,用于存储线程中待执行的函数
 * 此结构在线程自己的内核栈中位置不固定,
 * 用在switch_to时保存线程环境。
 * 实际位置取决于实际运行情况。
 ******************************************/
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

    // 线程在一个时间片上可运行的剩余时间（时钟数）
    uint8_t ticks;
    // 记录线程从运行开始，已经运行的时间（时钟数）
    uint32_t elapsed_ticks;
    // 用于连接就绪队列或者阻塞队列
    struct list_elem general_tag;
    // 用于连接存储所有线程的队列
    struct list_elem all_list_tag;
    /*
    进程自己页表的虚拟地址
    由于线程共享进程的虚拟地址空间
    因此线程PCB中此值为NULL
    */
    uint32_t pgdir;

    // 用于PCB结构体的边界标记
    uint32_t stack_magic;
};

/*初始化PCB*/
void init_thread(struct task_struct *pthread, char *name, int prio);
/*根据PCB信息，初始化线程栈的运行信息*/
void thread_create(struct task_struct *pthread, thread_func function, void *func_args);
/*根据线程栈的运行信息开始运行线程函数*/
struct task_struct *thread_start(char *name, int prio, thread_func function, void *func_args);

/*获取当前正在运行的线程的PCB*/
struct task_struct *running_thread(void);
// 初始化主线程
void thread_init(void);
/*时间片轮转调度函数*/
void schedule(void);
void thread_block(enum task_status status);
void thread_unblock(struct task_struct *pthread);
#endif