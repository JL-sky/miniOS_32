%include "boot.inc"

SECTION loader vstart=LOADER_BASE_ADDR
;初始化栈指针地址
LOADER_STACK_TOP equ LOADER_BASE_ADDR
jmp loader_start

;------------- 构建gdt及其内部的描述符 -------------
    ;使用GDT_BASE定义GDT的基址，dd是伪指令，用于定义双字变量，一个字是2个字节
    GDT_BASE: 
    ;定义GDT的第0个描述符（操作系统规定必须为全0）
        dd 0x00000000
        dd 0x00000000

    ;代码段描述符的低4字节部分，其中高两个字节表示段基址的0~15位,在这里定义为0x0000
    ;低两个字节表示段界限的0~15位，由于使用的是平坦模型，因此是0xFFFF
    CODE_DESC:  
        dd 0x0000FFFF
        dd DESC_CODE_HIGH4;段描述符的高4字节部分
    DATA_STACK_DESC: 
        dd 0x0000FFFF
        dd DESC_DATA_HIGH4

    ;定义显存段的描述符
    ;文本模式下的适配器地址为0xb8000~0xbffff,为了方便显存操作，显存段不使用平坦模型
    ;因此段基址为0xb8000,段大小为0xbffff-0xb8000=0x7fff,
    ;段粒度位4k，因此段界限的值为0x7fff/4k=7
    VIDEO_DESC: 
        dd 0x80000007
        dd DESC_VIDEO_HIGH4

    GDT_SIZE equ $-GDT_BASE
    GDT_LIMIT equ GDT_SIZE-1

    ;dq，定义四字变量，也就是8字节变量
    times 60 dq 0 ;此处预留60个描述符的空位
    
;------------- 构建选择子 -------------
    SELECTOR_CODE equ (0x0001<<3) + TI_GDT + RPL0
    SELECTOR_DATA equ (0x0002<<3) + TI_GDT + RPL0
    SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0


;------------- 定义gdtr（指向GDT的寄存器） -------------
    gdt_ptr dw GDT_LIMIT
            dd GDT_BASE
            
;------------- 以下是在进入保护模式之前的测试，在实模式下运行的 -------------
;------------- 加载器的显示信息 -------------
    loadermsg db '2 loader in real .'

loader_start:
;------------------------------------------
;INT 0x10 功能号：0x13 功能描述：打印字符串
;------------------------------------------
;输入：
    ;AH:功能号
    ;AL:显示输出方式
    ;   0——字符串中只含显示字符，显示属性在BH中，显示后光标位置不变
    ;   1——字符串中只含显示字符，显示属性在BH中，显示后光标位置改变
    ;   2——字符串中含显示字符和显示属性，显示后光标位置不变
    ;   3——字符串中含显示字符和显示属性，显示后光标位置改变
    ;BH:页码
    ;BL:属性
    ;CX:字符串长度
    ;(DH、DL):坐标（行、列）
    ;ES:BP 字符串地址
;无返回值
    mov sp,LOADER_BASE_ADDR
    mov bp,loadermsg
    mov cx,17
    mov ax,0x1301
    mov bx,0x001f;页号为0，蓝底粉红字
    mov dx,0x1800
    int 0x10

;------------- 准备进入保护模式 -------------
;1.打开A20
;2.加载gdt
;3.置cr0的PE位为1

    ;------------- 打开A20 -------------
    in al,0x92
    or al,0000_0010B
    out 0x92,al
    
    ;------------- 加载gdt -------------
    lgdt [gdt_ptr]

    ;------------- 置cr0的PE位为1 -------------
    mov eax,cr0
    or eax,0x00000001
    mov cr0,eax

    jmp dword SELECTOR_CODE:p_mode_start;刷新流水线

;------------- 以下是在保护模式下运行的  -------------
[bits 32]
p_mode_start:
     
    mov ax,SELECTOR_DATA           ;初始化段寄存器，将数据段的选择子分别放入各段寄存器
    mov ds,ax
    mov es,ax                      
    mov ss,ax

    mov esp,LOADER_STACK_TOP        ;初始化站栈指针，将栈指针地址放入bsp寄存器
    mov ax,SELECTOR_VIDEO           ;初始化显存段寄存器，显存段的选择子放入gs寄存器
    mov gs,ax

    mov byte [gs:160],'p'

    jmp $

