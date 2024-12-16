[bits 32]
%define ERROR_CODE nop
%define ZERO push 0
extern put_str                          ;使用外部函数(/kernel/print.S中的字符串打印函数)

section .data                           ;定义数据段
intr_str db "interrupt occur!",0xa,0    ;声明intr_str字符串数组(指针类型)，0xa表示换行符，0表示字符串的末尾标记符
 
global intr_entry_table                 ;定义中断处理程序数据，数组的元素值是中断处理程序，共33个
intr_entry_table:
 
;宏定义的语法格式为：
;       %macro 宏名 参数个数             ;参数顺序由占位符表示，如%1表示第一个参数，%2表示第二个参数
;           宏定义
;       %endmacro
%macro VECTOR 2                         ;定义VECTOR宏，该宏用于定义中断处理程序
                                        ;参数1表示中断向量号，参数2表示错误码

section .text                           ;中断处理程序的代码段
intr%1entry:                            ;这是一个标号，用于表示中断程序的入口地址，即中断程序的函数名
    %2                                  ;如果一个中断有错误码，则什么都不做，否则压入0作填充

;这段代码是为了模拟中断发生，如果我们中断调用后这段字符串打印出来了，说明我们的中断逻辑是正确的
    push intr_str                       ;压入put_str的调用参数
    call put_str
    add esp,4                           ;回收put_str函数压入的字符串参数的栈帧空间
                                        ;由于intr_str是一个指针类型，而该系统为32bit，因此该参数占用4个字节空间

;上述中断发生模拟结束后要结束中断 
    mov al,0x20                         ;中断结束命令EOI
    out 0xa0,al                         ;向主片发送OCW2,其中EOI位为1，告知结束中断，详见书p317
    out 0x20,al                         ;向从片发送OCW2,其中EOI位为1，告知结束中断

    add esp,4                           ;抛弃有错误码的中断（如果有）
    iret                                ;中断返回
    
section .data
    dd intr%1entry                      ;存储各个中断入口程序的地址，形成intr_entry_table数组,定义的地址是4字节，32位

%endmacro

; 调用之前写好的VECTOR宏来批量生成中断处理函数，
; 传入参数是中断号码与上面中断宏的%2步骤，这个步骤是什么都不做，还是压入0,详情看p303
VECTOR 0x00,ZERO
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