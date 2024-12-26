#include "timer.h"
#include "io.h"
#include "print.h"

#include "interrupt.h"
#include "thread.h"
#include "debug.h"

uint32_t ticks; // ticks是内核自中断开启以来总共的嘀嗒数

/* 时钟的中断处理函数 */
static void intr_timer_handler(void)
{
   struct task_struct *cur_thread = running_thread();
   // 检查栈是否溢出
   ASSERT(cur_thread->stack_magic == 0x20241221);
   // 记录此线程占用的cpu时间嘀
   cur_thread->elapsed_ticks++;
   // 从内核第一次处理时间中断后开始至今的滴哒数,内核态和用户态总共的嘀哒数
   ticks++;

   if (cur_thread->ticks == 0)
   { // 若进程时间片用完就开始调度新的进程上cpu
      schedule();
   }
   else
   { // 将当前进程的时间片-1
      cur_thread->ticks--;
   }
}

void timer_init()
{
   put_str("timer_init start\n");
   /* 设置8253的定时周期,也就是发中断的周期 */
   register_handler(0x20, intr_timer_handler);
   put_str("timer_init done\n");
}