[bits 32]
section .text
global switch_to
switch_to:
    ; 栈中此处是返回地址
    push esi
    push edi
    push ebx
    push ebp
    ; 取出栈中的参数cur,cur = [esp+20]
    ; 即当前已经时间片运行结束的线程PCB
    mov eax,[esp+20]
    ; 保存栈顶指针esp到当前时间片运行结束的线程PCB的self_kstack字段,
    ; self_kstack在task_struct中的偏移为0,
	; 所以直接往thread开头处存4字节便可。
    mov [eax],esp

    ; 取出栈中的参数next, next = [esp+24]
    ; 即要被换上CPU的线程PCB
    mov eax,[esp+24]
    ;让esp指向新线程PCB中的运行栈的栈顶指针
    mov esp,[eax]

    pop ebp
    pop ebx
    pop edi
    pop esi
    ; 返回到上面switch_to下面的那句注释的返回地址,
    ; 未由中断进入,第一次执行时会返回到kernel_thread
    ret		 				
				 					