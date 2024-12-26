TI_GDT equ 0
RPL0 equ 0
SELECTOR_VIDEO equ (0x0003<<3)+TI_GDT+RPL0;定义显存段选择子

section .data
put_int_buffer dq 0                                         ; 定义8字节缓冲区用于数字到字符的转换

;================= 字符串打印函数 ================= 
[bits 32]
section .text
global put_str

put_str:
    push ebx
    push ecx
    xor ecx,ecx
    mov ebx,[esp+12]                ;ebx存放字符串首地址
.goon:
    mov cl,[ebx]                    ;取出字符串的第一个字符，存放到cx寄存器中
    cmp cl,0                        ;判断是否到了字符串结尾，字符串结尾标志符'\0'即0
    jz .str_over
    push ecx                        ;压入put_char参数，调用put_char函数
    call put_char
    add esp,4                       ;回收栈空间
    inc ebx                         ;指向字符串的下一个字符
    jmp .goon
.str_over:
    pop ecx                         ;回收栈空间
    pop ebx                         ;回收栈空间
    ret


;================= 单字符打印函数 =================
[bits 32]
section .text
global put_char                     ;通过global关键字将put_char函数定义为全局符号
                                    ;使其对外部文件可见
;================= put_char函数实现 =================
put_char:
    pushad                          ;push all double,压入所有双字长的寄存器
                                    ;入栈顺序为eax->ecx->edx->ebx->esp->ebp->esi->edi
 
    mov ax,SELECTOR_VIDEO
    mov gs,ax                       ;为gs寄存器赋予显存段的选择子

;---------------- 获取光标的坐标位置 ----------------
    ; 以下代码用于获取光标的坐标位置(一维索引，如第一行的坐标范围为0~79,第二行为80~159)
    ; 其中光标的坐标位置存放在光标坐标寄存器中
    ; 其中索引为0eh的寄存器和索引为0fh的寄存器分别存放光标高8位和低8位
    ; 访问CRT controller寄存器组的寄存器，需要先往端口地址为0x03d4的寄存器写入索引
    ; 从端口地址为0x03d5的数据寄存器中读写数据
    mov dx,0x03d4                   ;将0x03d4的端口写入dx寄存器中
    mov al,0x0e                     ;将需要的索引值写入al寄存器中
    out dx,al                       ;向0x03d4端口写入0x0e索引
    mov dx,0x03d5                   
    in al,dx                        ;从0x03d5端口处获取光标高8位
    mov ah,al                       ;ax寄存器用于存放光标坐标，
                                    ;因此将光标坐标的高8位数据存放到ah中
;同上，以下代码获取光标坐标的低8位
    mov dx,0x03d4
    mov al,0x0f
    out dx,al
    mov dx,0x03d5
    in al,dx                        ;此时ax中就存放着读取到的光标坐标值

    mov bx,ax                       ;bx寄存器不仅是光标坐标值，同时也是下一个可打印字符的位置
                                    ;而我们习惯于bx作为基址寄存器，以后的处理都要基于bx寄存器
                                    ;因此才将获取到的光标坐标值赋值为bx
                                    
;---------------- 参数（待打印的字符）传递 ----------------
    mov ecx,[esp+36]                ;前边已经压入了8个双字（一个字2个字节）的寄存器，
                                    ;加上put_char函数的4字节返回地址
                                    ;所以待打印的字符在栈顶偏移36字节的位置

    cmp cl,0xd                      ;回车符处理
    jz .is_carriage_return
 
    cmp cl,0xa                      ;换行符处理
    jz .is_line_feed
 
    cmp cl,0x8                      ;退格键处理
    jz .is_backspace                
    jmp .put_other                  ;正常字符处理
 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;---------------- 退格键处理 ----------------
    ;处理思路：
        ;1.将光标位置减一
        ;2.将待删除的字符使用空格字符(ASCII:0x20)代替
.is_backspace:
    dec bx                          ;bx中存储光标的坐标位置，将光标坐标位置减去一，即模拟退格
    shl bx,1                        ;由于文本模式下一个字符占用两个字节(第一个字节表示字符的ASCII码，第二个字节表示字符的属性)，
                                    ;故光标位置乘以2(shl左移指令)就是光标处字符的第一个字节的偏移量
    mov byte[gs:bx],0x20            ;将空格键存入待删除字符处
    inc bx                          ;此时bx中存储的是字待删除字符的第一个字节位置，
                                    ;使用inc指令将bx加1后就是该字符的第二个字节的位置
    mov byte[gs:bx],0x07            ;将黑底白字(0x07)属性加入到该字符处
    shr bx,1                        ;bx除以2，恢复光标坐标位置
    jmp .set_cursor                 ;去设置光标位置, 这样光标位置才能真正在视觉上更新
 
;将cx指向的字符放入到光标处
.put_other:
    shl bx,1                        ;将光标坐标转换为内存偏移量
    mov byte[gs:bx],cl              ;将cx指向的字符放入到光标处
    inc bx                          ;bx指向当前字符的下一个字节处，存放当前字符属性
    mov byte[gs:bx],0x07            ;存放字符属性
    shr bx,1                        ;将内存偏移量恢复为光标坐标值
    inc bx                          ;bx指向下一个待写入字符位置
    cmp bx,2000                     ;80*25=2000，判断是否字符已经写满屏了
    jl .set_cursor                  ;更新光标坐标值
 
;---------------- 换行处理 ----------------
    ;思路：首先将光标移动到本行行首，之后再将光标移动到下一行行首
.is_line_feed:
.is_carriage_return:
    xor dx,dx
    ;将光标移动到本行行首
    mov ax,bx                       ;将光标值给ax，以作除法之用
    mov si,80                       
    div si                          ;除法操作，ax/si,结果ax存储商，dx存储余数
                                    ;每行80个字符，光标值除以80得出的余数便是当前光标所在行的字符个数
    sub bx,dx                       ;光标值减去光标当前行的字符个数，就将光标移动到本行行首的位置
 
    .is_carriage_return_end:
        add bx,80                   ;将光标移动到下一行行首
        cmp bx,2000                 ;屏幕的每屏的可显示80*25=2000个字符（25行，每行80个字符）
                                    ;这行代码和接下来jl .set_cursor这段代码的作用是
                                    ;判断当前光标值是否小于2000
                                    ;如果小于2000说明当前光标仍旧在本屏内，则代码直接跳转到.set_cursor更新光标值即可
                                    ;否则说明需要滚动屏幕了，继续向下执行滚屏部分的代码.roll_screen
    .is_line_feed_end:
        jl .set_cursor

;---------------- 滚屏处理 ----------------
    ;思路:屏幕行范围是0~24,滚屏的原理是将屏幕的1~24行搬运到0~23行,再将第24行用空格填充

    ; mosd指令会按照指定的次数复制每次 4 字节
    ; 源地址：由 SI（源数据位置寄存器）指向。
    ; 目标地址：由 DI（目标位置寄存器）指向。
    ; 计数器：由 CX（在 16 位模式下）或 ECX（在 32 位模式下）寄存器控制，表示要复制的数据块的元素个数（每个元素是 4 字节）。
    ; 每执行一次 movsd，SI 和 DI 会分别递增 4（因为是复制 32 位数据），ECX 会减 1，直到 ECX 变为 0。
.roll_screen:
    cld                             ;清除eflags寄存器的方向标志位DF，使得内存移动地址从低地址向高地址移动
                                    ;若方向标志位被设置，则字符串的内存移动地址从高地址向低地址移动
    mov ecx,960                     ;共移动2000-80=1920个字符，每个字符占2个字节，故共需移动1920*2=3840个字节
                                    ;movsd指令每次移动4个字节，故共需执行该指令3840/4=960次数
    mov esi,0xb80a0                 ;第1行行首地址，要复制的起始地址
    mov edi,0xb8000                 ;第0行行首地址，要复制的目的地址
    rep movsd                       ;rep(repeat)指令，重复执行movsd指令，执行的次数在ecx寄存器中
 
    ;将最后一行填充为空白
    mov ebx,3840                    ;更新光标位置为最后一行首字符第一个字节的位置
    mov ecx,80
    .cls:
        mov word[gs:ebx],0x0720     ;0x0720是黑底白字的空格键
        add ebx,2
        loop .cls
        mov bx,1920                 ;将光标值重置为1920,最后一行的首字符.
;---------------- 更新光标值 ----------------
.set_cursor:
    					                                    ;将光标设为bx值
    ;1 先设置高8位
    mov dx, 0x03d4			                                ;索引寄存器
    mov al, 0x0e				                            ;用于提供光标位置的高8位
    out dx, al
    mov dx, 0x03d5			                                ;通过读写数据端口0x3d5来获得或设置光标位置 
    mov al, bh
    out dx, al
 
    ;2 再设置低8位
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5 
    mov al, bl
    out dx, al
 
.put_char_done:
    popad
    ret

;--------------------  打印数字  -----------------------
;思路：将小端字节序的数字变成对应的ascii后，倒置
;输入：栈中参数为待打印的数字
;输出：在屏幕上打印16进制数字,并不会打印前缀0x,如打印10进制15时，只会直接打印f，不会是0xf
;------------------------------------------------------------------------------------------
global put_int
put_int:
   pushad
   mov ebp, esp
   mov eax, [ebp+4*9]		                                ; call的返回地址占4字节+pushad的8个4字节，现在eax中就是要显示的32位数值
   mov edx, eax                                             ;ebx中现在是要显示的32位数值
   mov edi, 7                                               ; 指定在put_int_buffer中初始的偏移量，也就是把栈中第一个字节取出放入buffer最后一个位置，第二个字节放入buff倒数第二个位置
   mov ecx, 8			                                    ; 32位数字中,16进制数字的位数是8个
   mov ebx, put_int_buffer                                  ;ebx现在存储的是buffer的起始地址
 
                                                            ;将32位数字按照16进制的形式从低位到高位逐个处理,共处理8个16进制数字
.16based_4bits:			                                    ; 每4位二进制是16进制数字的1位,遍历每一位16进制数字
   and edx, 0x0000000F		                                ; 解析16进制数字的每一位。and与操作后,edx只有低4位有效
   cmp edx, 9			                                    ; 数字0～9和a~f需要分别处理成对应的字符
   jg .is_A2F 
   add edx, '0'			                                    ; ascii码是8位大小。add求和操作后,edx低8位有效。
   jmp .store
.is_A2F:
   sub edx, 10			                                    ; A~F 减去10 所得到的差,再加上字符A的ascii码,便是A~F对应的ascii码
   add edx, 'A'
 
                                                            ;将每一位数字转换成对应的字符后,按照类似“大端”的顺序存储到缓冲区put_int_buffer
                                                            ;高位字符放在低地址,低位字符要放在高地址,这样和大端字节序类似,只不过咱们这里是字符序.
.store:
   mov [ebx+edi], dl		                                ; 此时dl中是数字对应的字符的ascii码
   dec edi                                                  ;edi是表示在buffer中存储的偏移，现在向前移动
   shr eax, 4                                               ;eax中是完整存储了这个32位数值，现在右移4位，处理下一个4位二进制表示的16进制数字
   mov edx, eax                                             ;把eax中的值送入edx，让ebx去处理
   loop .16based_4bits
 
                                                            ;现在put_int_buffer中已全是字符,打印之前,
                                                            ;把高位连续的字符去掉,比如把字符00000123变成123
.ready_to_print:
   inc edi			                                        ; 此时edi退减为-1(0xffffffff),加1使其为0
.skip_prefix_0:                                             ;跳过前缀的连续多个0
   cmp edi,8			                                    ; 若已经比较第9个字符了，表示待打印的字符串为全0 
   je .full0 
                                                            ;找出连续的0字符, edi做为非0的最高位字符的偏移
.go_on_skip:   
   mov cl, [put_int_buffer+edi]
   inc edi
   cmp cl, '0' 
   je .skip_prefix_0		                                ; 继续判断下一位字符是否为字符0(不是数字0)
   dec edi			                                        ;edi在上面的inc操作中指向了下一个字符,若当前字符不为'0',要恢复edi指向当前字符		       
   jmp .put_each_num
 
.full0:
   mov cl,'0'			                                    ; 输入的数字为全0时，则只打印0
.put_each_num:
   push ecx			                                        ; 此时cl中为可打印的字符
   call put_char
   add esp, 4
   inc edi			                                        ; 使edi指向下一个字符
   mov cl, [put_int_buffer+edi]	                            ; 获取下一个字符到cl寄存器
   cmp edi,8                                                ;当edi=8时，虽然不会去打印，但是实际上已经越界访问缓冲区了
   jl .put_each_num
   popad
   ret

global set_cursor
set_cursor:
   pushad
   mov bx, [esp+36]
															;;;;;;; 1 先设置高8位 ;;;;;;;;
   mov dx, 0x03d4			  								;索引寄存器
   mov al, 0x0e				  								;用于提供光标位置的高8位
   out dx, al
   mov dx, 0x03d5			  								;通过读写数据端口0x3d5来获得或设置光标位置 
   mov al, bh
   out dx, al

															;;;;;;; 2 再设置低8位 ;;;;;;;;;
   mov dx, 0x03d4
   mov al, 0x0f
   out dx, al
   mov dx, 0x03d5 
   mov al, bl
   out dx, al
   popad
   ret