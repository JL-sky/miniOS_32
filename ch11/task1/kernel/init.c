#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "timer.h"
#include "memory.h"
#include "thread.h"
#include "console.h"
#include "keyboard.h"
#include "tss.h"

/*负责初始化所有模块 */
void init_all()
{
    put_str("init_all\n");
    idt_init();      // 初始化中断
    mem_init();      // 初始化内存管理系统
    thread_init();   // 初始化线程相关结构
    timer_init();    // 时钟中断初始化
    console_init();  // 终端初始化
    keyboard_init(); // 键盘中断初始化
    tss_init();      // 初始化tss
}