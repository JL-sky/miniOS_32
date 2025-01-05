#ifndef __USERPROG_TSS_H
#define __USERPROG_TSS_H

#include "thread.h"
/*更新TSS中的esp0的值，让它指向线程/进程的0级栈*/
void update_tss_esp(struct task_struct *pthread);
/* 在gdt中创建tss并重新加载gdt */
void tss_init(void);
#endif