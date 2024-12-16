#include "interrupt.h" //里面定义了intr_handler类型
#include "stdint.h"    //各种uint_t类型
#include "global.h"    //里面定义了选择子
#include "io.h"        //封装了中断控制器代理的操作函数
#include "print.h"

#define PIC_M_CTRL 0x20 // 这里用的可编程中断控制器是8259A,主片的控制端口是0x20
#define PIC_M_DATA 0x21 // 主片的数据端口是0x21
#define PIC_S_CTRL 0xa0 // 从片的控制端口是0xa0
#define PIC_S_DATA 0xa1 // 从片的数据端口是0xa1

#define IDT_DESC_CNT 0x21 // 支持的中断描述符个数33

// 定义中断描述符结构体
struct gate_desc
{
    uint16_t func_offset_low_word;  // 中断处理程序在目标代码段的偏移量（低字）
    uint16_t selector;              // 选择子字段
    uint8_t dcount;                 // 此项为双字计数字段，是门描述符中的第4字节。这个字段无用
    uint8_t attribute;              // 属性字段（包含P位、DPL位、S位、TYPE位）
    uint16_t func_offset_high_word; // 中断处理程序在目标代码段的偏移量（高字）
};

// 定义中断门描述符结构体数组，形成中断描述符表idt，该数组中的元素是中断描述符
static struct gate_desc idt[IDT_DESC_CNT];

/*
intr_handler为void*类型
intr_entry_table保存的是kernel.S定义的33个中断处理程序的入口地址（函数地址）
*/
extern intr_handler intr_entry_table[IDT_DESC_CNT];

// 静态函数声明，该函数用于构建中断描述符，将上述定义的中断描述符结构体按照字段说明进行填充即可
static void make_idt_desc(struct gate_desc *p_gdesc, uint8_t attr, intr_handler function);

/*
函数功能：构建中断描述符
函数实现：按照中断描述符结构体定义填充字段
参数：中断门描述符地址，属性，中断处理函数
*/
static void make_idt_desc(struct gate_desc *p_gdesc, uint8_t attr, intr_handler function)
{
    p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000FFFF;
    p_gdesc->selector = SELECTOR_K_CODE;
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attr;
    p_gdesc->func_offset_high_word = ((uint32_t)function & 0xFFFF0000) >> 16;
}

/*
函数功能：构建中断描述符表idt
函数实现：循环调用make_idt_desc构建中断描述符，形成中断描述符表idt
参数：中断描述符表中的某个中断描述符地址，属性字段，中断处理函数地址
*/
static void idt_desc_init(void)
{
    int i;
    for (i = 0; i < IDT_DESC_CNT; i++)
    {
        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
    }
    put_str("   idt_desc_init done\n");
}

/*初始化可编程中断控制器8259A*/
static void pic_init()
{
    /* 初始化主片 */
    outb(PIC_M_CTRL, 0x11); // ICW1: 边沿触发,级联8259, 需要ICW4.
    outb(PIC_M_DATA, 0x20); // ICW2: 起始中断向量号为0x20,也就是IR[0-7] 为 0x20 ~ 0x27.
    outb(PIC_M_DATA, 0x04); // ICW3: IR2接从片.
    outb(PIC_M_DATA, 0x01); // ICW4: 8086模式, 正常EOI

    /* 初始化从片 */
    outb(PIC_S_CTRL, 0x11); // ICW1: 边沿触发,级联8259, 需要ICW4.
    outb(PIC_S_DATA, 0x28); // ICW2: 起始中断向量号为0x28,也就是IR[8-15] 为 0x28 ~ 0x2F.
    outb(PIC_S_DATA, 0x02); // ICW3: 设置从片连接到主片的IR2引脚
    outb(PIC_S_DATA, 0x01); // ICW4: 8086模式, 正常EOI

    /* 打开主片上IR0,也就是目前只接受时钟产生的中断 */
    outb(PIC_M_DATA, 0xfe);
    outb(PIC_S_DATA, 0xff);

    put_str("pic_init done\n");
}

/*完成有关中断的所有初始化工作*/
void idt_init()
{
    put_str("idt_init start\n");
    idt_desc_init(); // 构建中段描述符表
    pic_init();      // 初始化中断控制器，只接受来自时钟中断的信号

    /* 加载idt */
    uint64_t idt_operand = (((uint64_t)(uint32_t)idt << 16) | (sizeof(idt) - 1)); // 定义要加载到IDTR寄存器中的值
    asm volatile("lidt %0" : : "m"(idt_operand));
    put_str("idt_init done\n");
}
