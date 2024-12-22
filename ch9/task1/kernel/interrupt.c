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

/*eflags寄存器中的if位的掩码*/
#define EFLAGS_IF 0x00000200 // 表示eflags寄存器中的if位为1
                             // 将该宏与当前的if位（中断状态）进行与操作，即可获取当前的中断状态

/*
功能：获取eflags寄存器中的值
pushfl将eflags寄存器中的值压入栈顶，再使用popl将栈顶的值弹出到EFLAG_VAR所在内存中
该约束自然用表示内存的字母，但是内联汇编中没有专门表示约束内存的字母，所以只能用g
代表可以是任意寄存器，内存或立即数
*/
#define GET_EFLAGS(EFLAG_VAR) asm volatile("pushfl;pop %0" : "=g"(EFLAG_VAR))

// 定义中断描述符结构体
struct gate_desc
{
    uint16_t func_offset_low_word;  // 函数地址低字
    uint16_t selector;              // 选择子字段
    uint8_t dcount;                 // 此项为双字计数字段，是门描述符中的第4字节。这个字段无用
    uint8_t attribute;              // 属性字段
    uint16_t func_offset_high_word; // 函数地址高字
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

// 存储中断/异常的名字
char *intr_name[IDT_DESC_CNT];
/*
定义中断处理程序数组
在kernel.S中定义的intrXXentry只是中断处理程序的入口,最终调用的是ide_table中的处理程序
*/
intr_handler idt_table[IDT_DESC_CNT];

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
函数功能：中断描述符表idt
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
    put_str("idt_desc_init done\n");
}

#if 1
/*用c语言定义中断处理函数的具体实现，不再用汇编语言在kernel.S中定义中断处理函数的实现*/
// 参数：中断向量号
static void general_intr_handler(uint8_t vec_nr)
{
    // 伪中断向量，无需处理
    if (vec_nr == 0x27 || vec_nr == 0x2f)
    {
        return;
    }
    /*具体的终端处理逻辑*/
    // put_str("int vector:0x");
    // put_int(vec_nr);
    // put_char('\n');
}
#endif

/* 完成一般中断处理函数注册及异常名称注册 */
static void exception_init(void)
{ // 完成一般中断处理函数注册及异常名称注册
    int i;
    for (i = 0; i < IDT_DESC_CNT; i++)
    {
        /* idt_table数组中的函数是在进入中断后根据中断向量号调用的,
        见kernel/kernel.S的call [idt_table + %1*4] */
        idt_table[i] = general_intr_handler; // 默认为general_intr_handler
                                             // 以后会由register_handler来注册具体处理函数。
        intr_name[i] = "unknown";            // 先统一赋值为unknown
    }
    intr_name[0] = "#DE Divide Error";
    intr_name[1] = "#DB Debug Exception";
    intr_name[2] = "NMI Interrupt";
    intr_name[3] = "#BP Breakpoint Exception";
    intr_name[4] = "#OF Overflow Exception";
    intr_name[5] = "#BR BOUND Range Exceeded Exception";
    intr_name[6] = "#UD Invalid Opcode Exception";
    intr_name[7] = "#NM Device Not Available Exception";
    intr_name[8] = "#DF Double Fault Exception";
    intr_name[9] = "Coprocessor Segment Overrun";
    intr_name[10] = "#TS Invalid TSS Exception";
    intr_name[11] = "#NP Segment Not Present";
    intr_name[12] = "#SS Stack Fault Exception";
    intr_name[13] = "#GP General Protection Exception";
    intr_name[14] = "#PF Page-Fault Exception";
    // intr_name[15] 第15项是intel保留项，未使用
    intr_name[16] = "#MF x87 FPU Floating-Point Error";
    intr_name[17] = "#AC Alignment Check Exception";
    intr_name[18] = "#MC Machine-Check Exception";
    intr_name[19] = "#XF SIMD Floating-Point Exception";
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

/* 获取当前中断状态 */
enum intr_status intr_get_status()
{
    uint32_t eflags = 0;
    GET_EFLAGS(eflags);
    return (EFLAGS_IF & eflags) ? INTR_ON : INTR_OFF;
}

/* 开中断并返回开中断前的状态*/
enum intr_status intr_enable()
{
    enum intr_status old_status;
    if (INTR_ON == intr_get_status())
    {
        old_status = INTR_ON;
        return old_status;
    }
    else
    {
        old_status = INTR_OFF;
        asm volatile("sti"); // 开中断,sti指令将IF位置1
        return old_status;
    }
}

/* 关中断,并且返回关中断前的状态 */
enum intr_status intr_disable()
{
    enum intr_status old_status;
    if (INTR_ON == intr_get_status())
    {
        old_status = INTR_ON;
        asm volatile("cli" : : : "memory"); // 关中断,cli指令将IF位置0
                                            // cli指令不会直接影响内存。然而，从一个更大的上下文来看，禁用中断可能会影响系统状态，
                                            // 这个状态可能会被存储在内存中。所以改变位填 "memory" 是为了安全起见，确保编译器在生成代码时考虑到这一点。
        return old_status;
    }
    else
    {
        old_status = INTR_OFF;
        return old_status;
    }
}

/* 将中断状态设置为status */
enum intr_status intr_set_status(enum intr_status status)
{
    return status & INTR_ON ? intr_enable() : intr_disable(); // enable与disable函数会返回旧中断状态
}

/*完成有关中断的所有初始化工作*/
void idt_init()
{
    put_str("idt_init start\n");
    idt_desc_init();  // 完成中段描述符表的构建
    exception_init(); // 异常名初始化并注册通用的中断处理函数
    pic_init();       // 设定化中断控制器，只接受来自时钟中断的信号

    /* 加载idt */
    uint64_t idt_operand = (((uint64_t)(uint32_t)idt << 16) | (sizeof(idt) - 1)); // 定义要加载到IDTR寄存器中的值
    asm volatile("lidt %0" : : "m"(idt_operand));
    put_str("idt_init done\n");
}
