[bits 32]
section .text
global switch_to
switch_to:
   ;栈中此处是返回地址
   ;这4条就是对应压入线程栈中预留的ABI标准要求保存的，esp会保存在其他地方      
   push esi                      	
   push edi
   push ebx
   push ebp

   mov eax, [esp + 20]		      	; 得到栈中的参数cur,即当前pcb, cur = [esp+20]
   mov [eax], esp                	; 记录当前线程运行到哪了 
                                    ; 将栈顶指针esp保存到当前task_struct的self_kstack字段,
                                    ; self_kstack在task_struct中的偏移为0,
                                    ; 所以直接往thread开头处存4字节便可。

;------------------  以上是备份当前线程的环境，下面是恢复下一个线程的环境  ----------------
   mov eax, [esp + 24]		 		; 得到栈中的参数next，即要切换的pcb, next = [esp+24]
   mov esp, [eax]		 			   ; pcb的第一个成员是self_kstack成员,用来记录0级栈顶指针,
				 					      ; 用来上cpu时恢复0级栈,0级栈中保存了进程或线程所有信息,包括3级栈指针
   pop ebp
   pop ebx
   pop edi
   pop esi
   ret				 				   ; 返回到上面switch_to下面的那句注释的返回地址,
				 					      ; 未由中断进入,第一次执行时会返回到kernel_thread
