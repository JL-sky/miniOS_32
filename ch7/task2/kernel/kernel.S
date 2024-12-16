[bits 32]
%define ERROR_CODE nop
%define ZERO push 0
 
extern idt_table

section .data                           ;定义数据段
 
global intr_entry_table                 ;定义中断处理程序数据，数组的元素值是中断处理程序，共33个
intr_entry_table:

;宏定义的语法格式为：
;       %macro 宏名 参数个数
;           宏定义
;       %endmacro
%macro VECTOR 2                         ;定义VECTOR宏，该宏用于定义中断处理程序
section .text                           ;中断处理程序的代码段
intr%1entry:                            ;这是一个标号，用于表示中断程序的入口地址，即中断程序的函数名
    %2                                  ;压入中断错误码（如果有）
 
    push ds                             ; 以下是保存上下文环境
    push es
    push fs
    push gs
    pushad
 
                                        ; 如果是从片上进入的中断,除了往从片上发送EOI外,还要往主片上发送EOI 
    mov al,0x20                         ; 中断结束命令EOI
    out 0xa0,al                         ;向主片发送OCW2,其中EOI位为1，告知结束中断，详见书p317
    out 0x20,al                         ;向从片发送OCW2,其中EOI位为1，告知结束中断
 
    push %1			                    ; 不管idt_table中的目标程序是否需要参数,都一律压入中断向量号,调试时很方便
    call [idt_table + %1*4]             ; 调用idt_table中的C版本中断处理函数
    jmp intr_exit
    
section .data
    dd intr%1entry                      ;存储各个中断入口程序的地址，形成intr_entry_table数组,定义的地址是4字节，32位
%endmacro
 
section .text
global intr_exit
intr_exit:
                                        ; 以下是恢复上下文环境
    add esp, 4			                ; 跳过中断号
    popad
    pop gs
    pop fs
    pop es
    pop ds
    add esp, 4			                 ;对于会压入错误码的中断会抛弃错误码（这个错误码是执行中断处理函数之前CPU自动压入的），对于不会压入错误码的中断，就会抛弃上面push的0                    
    iretd				                 ; 从中断返回,32位下iret等同指令iretd
 
 
VECTOR 0x00,ZERO                         ;调用之前写好的宏来批量生成中断处理函数，传入参数是中断号码与上面中断宏的%2步骤，这个步骤是什么都不做，还是压入0看p303
VECTOR 0x01,ZERO
VECTOR 0x02,ZERO
VECTOR 0x03,ZERO 
VECTOR 0x04,ZERO
VECTOR 0x05,ZERO
VECTOR 0x06,ZERO
VECTOR 0x07,ZERO 
VECTOR 0x08,ERROR_CODE
VECTOR 0x09,ZERO
VECTOR 0x0a,ERROR_CODE
VECTOR 0x0b,ERROR_CODE 
VECTOR 0x0c,ZERO
VECTOR 0x0d,ERROR_CODE
VECTOR 0x0e,ERROR_CODE
VECTOR 0x0f,ZERO 
VECTOR 0x10,ZERO
VECTOR 0x11,ERROR_CODE
VECTOR 0x12,ZERO
VECTOR 0x13,ZERO 
VECTOR 0x14,ZERO
VECTOR 0x15,ZERO
VECTOR 0x16,ZERO
VECTOR 0x17,ZERO 
VECTOR 0x18,ERROR_CODE
VECTOR 0x19,ZERO
VECTOR 0x1a,ERROR_CODE
VECTOR 0x1b,ERROR_CODE 
VECTOR 0x1c,ZERO
VECTOR 0x1d,ERROR_CODE
VECTOR 0x1e,ERROR_CODE
VECTOR 0x1f,ZERO 
VECTOR 0x20,ZERO