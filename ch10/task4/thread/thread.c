#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"

#include "print.h"
#define PG_SIZE 4096

// 主线程PCB
struct task_struct *main_thread;
// 线程就绪队列
struct list thread_ready_list;
// 存储所有任务的队列
struct list thread_all_list;
// 存储临时队列节点
static struct list_elem *thread_tag;
/*线程切换函数，在switch.S中实现*/
extern void switch_to(struct task_struct *cur, struct task_struct *next);

static void kernel_thread(thread_func *function, void *func_args)
{
    /* 执行function前要开中断,避免后面的时钟中断被屏蔽,而无法调度其它线程 */
    intr_enable();
    function(func_args);
}

/*初始化PCB*/
void init_thread(struct task_struct *pthread, char *name, int prio)
{
    memset(pthread, 0, sizeof(*pthread));
    strcpy(pthread->name, name);
    // pthread->status = TASK_RUNNGING;

    if (pthread == main_thread)
        pthread->status = TASK_RUNNING;
    else
        pthread->status = TASK_READY;
    pthread->priority = prio;

    // 初始化线程在一个时间片上可运行的剩余时间
    pthread->ticks = prio;
    // 记录线程从运行开始，已经运行的时间
    pthread->elapsed_ticks = 0;
    /*
    线程没有自己的地址空间
    进程的pcb这一项才有用，指向自己的页表虚拟地址
    */
    pthread->pgdir = (uint32_t)NULL;

    /*
    一个线程的栈空间分配一页空间，将PCB放置在栈底
    pthread是申请的一页空间的起始地址，因此加上一页的大小，就是栈顶指针
    */
    pthread->self_kstack = (uint32_t *)((uint32_t)pthread + PG_SIZE);
    /*PCB的边界标记，防止栈顶指针覆盖掉PCB的内容*/
    pthread->stack_magic = 0x20241221;
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
    /*
    1.分配一页的空间给线程作为线程执行的栈空间
    */
    struct task_struct *thread = get_kernel_pages(1);
    /*
    2.初始化PCB，PCB里存放了线程的基本信息以及线程栈的栈顶指针
    */
    init_thread(thread, name, prio);
    /*
    3.根据线程栈的栈顶指针，初始化线程栈，也就是初始化线程的运行信息
    比如线程要执行的函数，以及函数参数
    */
    thread_create(thread, function, func_args);

    /*
    4.将准备好PCB和运行信息的线程插入到就绪队列和存储所有任务的队列
    */
    ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
    list_append(&thread_ready_list, &thread->general_tag);
    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);

    return thread;
}

/*获取当前正在运行的线程的PCB*/
struct task_struct *running_thread(void)
{
    uint32_t esp;
    // 获取当前栈顶指针地址
    asm("mov %%esp,%0" : "=g"(esp));
    /*
    由于栈顶指针是在一页内存空间内，而PCB存储在这一页空间的起始地址处
    因此栈顶指针地址取整得到的这一页空间的起始地址就是PCB的起始地址
    */
    return (struct task_struct *)(esp & 0xfffff000);
}
static void make_main_thread(void)
{
    /* 因为main线程早已运行,咱们在loader.S中进入内核时的mov esp,0xc009f000,
就是为其预留了pcb,地址为0xc009e000,因此不需要通过get_kernel_page另分配一页*/
    main_thread = running_thread();
    init_thread(main_thread, "main", 31);
    /* main函数是当前线程,当前线程不在thread_ready_list中,
     * 所以只将其加在thread_all_list中. */
    ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
    list_append(&thread_all_list, &main_thread->all_list_tag);
}
// 初始化主线程
void thread_init(void)
{
    put_str("thread_init start\n");
    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    make_main_thread();
    put_str("thread_init done\n");
}
/*时间片轮转调度函数*/
void schedule(void)
{
    ASSERT(intr_get_status() == INTR_OFF);
    /*在关中断的情况下
    把当前时间片已经用完的线程换到就绪队列尾
    然后从就绪队列头取出一个新的线程换上执行新的时间片
    */
    struct task_struct *cur = running_thread();
    if (cur->status == TASK_RUNNING)
    {
        ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
        list_append(&thread_ready_list, &cur->general_tag);
        cur->ticks = cur->priority;
        cur->status = TASK_READY;
    }
    else
    {
        /* 若此线程需要某事件发生后才能继续上cpu运行,
          不需要将其加入队列,因为当前线程不在就绪队列中。*/
    }

    /* 将thread_ready_list队列中的第一个就绪线程弹出,准备将其调度上cpu. */
    ASSERT(!list_empty(&thread_ready_list));
    thread_tag = NULL; // thread_tag清空
    thread_tag = list_pop(&thread_ready_list);
    struct task_struct *next = elem2entry(struct task_struct, general_tag, thread_tag);
    next->status = TASK_RUNNING;
    switch_to(cur, next);
}
void thread_block(enum task_status status)
{
    ASSERT(((status == TASK_BLOCKED) || (status == TASK_HANGING) || (status == TASK_WAITING)));
    enum intr_status old_status = intr_get_status();
    /*修改当前线程状态为阻塞态
    然后调用调度器从就绪队列摘取一块新线程执行*/
    struct task_struct *cur_pthread = running_thread();
    cur_pthread->status = status;
    schedule();

    intr_set_status(old_status);
}

void thread_unblock(struct task_struct *pthread)
{
    enum intr_status old_status = intr_get_status();
    /*修改PCB状态为就绪态，同时插入就绪队列头部，优先调用*/
    enum task_status status = pthread->status;
    ASSERT(((status == TASK_BLOCKED) || (status == TASK_HANGING) || (status == TASK_WAITING)));
    if (status != TASK_READY)
    {
        if (elem_find(&thread_ready_list, &pthread->general_tag))
            PANIC("thread_unblock: blocked thread in ready_list\n");
        list_append(&thread_ready_list, &pthread->general_tag);
        pthread->status = TASK_READY;
    }

    intr_set_status(old_status);
}