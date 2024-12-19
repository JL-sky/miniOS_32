#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "memory.h"

/*初始化所有模块*/
void init_all()
{
    put_str("init_all\n");
    // 初始化中断
    idt_init();
    // 初始化内存管理系统
    mem_init();
}