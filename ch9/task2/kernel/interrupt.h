#ifndef __KERNEL_INTERRUPT_H
#define __KERNEL_INTERRUPT_H
#include "stdint.h"
typedef void *intr_handler; // 将intr_handler定义为void*同类型
void idt_init(void);

/*定义中断状态*/
enum intr_status
{
    INTR_OFF, // 表示中断关闭
    INTR_ON   // 表示中断打开
};
enum intr_status intr_get_status(void);
enum intr_status intr_set_status(enum intr_status);
enum intr_status intr_enable(void);
enum intr_status intr_disable(void);
// 中断处理程序注册
void register_handler(uint8_t vector_no, intr_handler function);
#endif