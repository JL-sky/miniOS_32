#include "print.h"
#include "init.h"
void main(void)
{
    put_str("I am kernel\n");
    init_all();          // 初始化中断，同时只放行时钟中断
    asm volatile("sti"); // 为演示中断处理,在此临时开中断,sti是开中断指令
    while (1)
        ;
}