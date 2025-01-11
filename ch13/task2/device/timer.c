#include "timer.h"
#include "io.h"
#include "print.h"

#include "interrupt.h"
#include "thread.h"
#include "debug.h"

#define IRQ0_FREQUENCY 100 // 定义我们想要的中断发生频率，100HZ
#define mil_seconds_per_intr (1000 / IRQ0_FREQUENCY)

uint32_t ticks; // ticks是内核自中断开启以来总共的嘀嗒数

/* 时钟的中断处理函数 */
static void intr_timer_handler(void)
{
   struct task_struct *cur_thread = running_thread();

   ASSERT(cur_thread->stack_magic == 0x20241221); // 检查栈是否溢出

   cur_thread->elapsed_ticks++; // 记录此线程占用的cpu时间嘀
   ticks++;                     // 从内核第一次处理时间中断后开始至今的滴哒数,内核态和用户态总共的嘀哒数

   if (cur_thread->ticks == 0)
   { // 若进程时间片用完就开始调度新的进程上cpu
      schedule();
   }
   else
   { // 将当前进程的时间片-1
      cur_thread->ticks--;
   }
}

/* 初始化PIT8253 */
void timer_init()
{
   put_str("timer_init start\n");
   register_handler(0x20, intr_timer_handler);
   put_str("timer_init done\n");
}

/* 以tick为单位的sleep,任何时间形式的sleep会转换此ticks形式 */
static void ticks_to_sleep(uint32_t sleep_ticks)
{
   uint32_t start_tick = ticks;
   /* 若间隔的ticks数不够便让出cpu */
   while (ticks - start_tick < sleep_ticks)
   {
      thread_yield();
   }
}

/* 以毫秒为单位的sleep   1秒= 1000毫秒 */
void mtime_sleep(uint32_t m_seconds)
{
   uint32_t sleep_ticks = DIV_ROUND_UP(m_seconds, mil_seconds_per_intr);
   ASSERT(sleep_ticks > 0);
   ticks_to_sleep(sleep_ticks);
}
