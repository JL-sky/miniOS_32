section .data
    sys_call db 'Hello from syscall write!', 0xa  ; 0xa 为换行符
    sys_call_len equ $ - sys_call

    str_c db 'Hello from simu write!',0xa
    str_c_len equ $ - str_c

section .text
    global _start

_start:
;------- 模拟c语言的系统调用write --------
    push str_c_len
    push str_c
    push 1

    call simu_write
    add esp,12
    
;------ 调用 sys_write ---------
    mov eax, 4              ; sys_write 系统调用号
    mov ebx, 1              ; 文件描述符 1（标准输出）
    mov ecx, sys_call       ; 消息的地址
    mov edx, sys_call_len   ; 消息的长度
    int 0x80                ; 执行系统调用

    ; 程序结束
    mov eax, 1              ; sys_exit 系统调用号
    xor ebx, ebx            ; 退出码 0
    int 0x80                ; 执行系统调用

simu_write:
    push ebp
    mov ebp,esp
    
    mov eax,4
    mov ebx,[ebp+8]
    mov ecx,[ebp+12]
    mov edx,[ebp+16]
    int 0x80

    pop ebp
    ret
