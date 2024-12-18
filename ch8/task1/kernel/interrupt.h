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

/* 获取当前中断状态 */
enum intr_status intr_get_status(void);
/* 开中断并返回开中断前的状态*/
enum intr_status intr_enable(void);
/* 关中断,并且返回关中断前的状态 */
enum intr_status intr_disable(void);
/* 将中断状态设置为status */
enum intr_status intr_set_status(enum intr_status);
#endif