%include "boot.inc"
SECTION loader vstart=LOADER_BASE_ADDR
;初始化栈指针地址
LOADER_STACK_TOP equ LOADER_BASE_ADDR

;------------- 构建gdt及其内部的描述符 -------------
    GDT_BASE: 
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
    times 60 dq 0 ;此处预留60个描述符的空位
    
;------------- 构建选择子 -------------
    SELECTOR_CODE equ (0x0001<<3) + TI_GDT + RPL0
    SELECTOR_DATA equ (0x0002<<3) + TI_GDT + RPL0
    SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0

    total_mem_bytes dd 0    ;total_mem_bytes用于保存最终获取到的内存容量，为4个字节
                            ;由于loader程序的加载地址为0x900，而loader.bin的文件头大小为0x200
                            ;（4个gdt段描述符（8B）加上60个dp（8B）填充字，故64*8=512B），
                            ;故total_mem_bytes在内存中的地址为0x900+0x200=0xb00
                            ;该地址将来在内核中会被用到


;------------- 定义gdtr（指向GDT的寄存器） -------------
    gdt_ptr dw GDT_LIMIT
            dd GDT_BASE

    ards_buf times 244 db 0 ;开辟一块缓冲区，用于记录返回的ARDS结构体，
                            ;该定义语句事实上是定义了一个数组ards_buf[244]
                            ;244是因为total_mem_bytes(4)+gdt_ptr(6)+244+ards_nr(2)=256,即0x100
                            ;这样loader_start的在文件内的偏移地址就是0x100+0x200=0x300

    ards_nr dw 0            ;用于记录ards结构体数量


;------------------------------------------
;INT 0x15 功能号：0xe820 功能描述：获取内存容量，检测内存
;------------------------------------------
;输入：
    ;EAX:功能号，0xE820，但调用返回时eax会被填入一串ASCII值
    ;EBX:ARDS后续值
    ;ES:di:ARDS缓冲区，BIOS将获取到的内存信息存到此寄存器指向的内存，每次都以ARDS格式返回
    ;ECX:ARDS结构的字节大小，20
    ;EDX:固定为签名标记，0x534d4150

;返回值
    ;CF：若cf为0，表示未出错，cf为1，表示调用出错
    ;EAX：字符串SMAP的ASCII码值，0x534d4150
    ;ES:di:ARDS缓冲区，BIOS将获取到的内存信息存到此寄存器指向的内存，每次都以ARDS格式返回
    ;ECX:ARDS结构的字节大小，20
    ;EBX:ARDS后续值，即下一个ARDS的位置。
    ;每次BIOS中断返回后，BIOS会更新此值，BIOS会通过此值找到下一个待返回的ARDS结构。
    ;在cf位为0的情况下，若返回后的EBX值为0，表示这是最后一个ARDS结构

loader_start:
    xor ebx,ebx             ;第一次调用时，要将ebx清空置为0，此处使用的是异或运算置0
    mov edx,0x534d4150
    mov di,ards_buf         ;di存储缓冲区地址，即指向缓冲区首地址

.e820_mem_get_loop:
    mov eax,0x0000e820
    mov ecx,20              ;一个ards结构体的大小
    int 0x15                ;调用0x15中断函数，返回的ards结构体被返回给di指向的缓冲区中
    add di,cx               ;使di增加20字节指向缓冲区中下一个的ARDS结构位置
    inc word [ards_nr]      ;inc（increment增加）指令表示将内存中的操作数增加一，此处用于记录返回的ARDS数量
    cmp ebx,0               ;比较ebx中的值是否为0
    jnz .e820_mem_get_loop  ;若ebx不为0，则继续进行循环获取ARDS，
                            ;若为0说明已经获取到最后一个ards，则退出循环

    mov cx,[ards_nr]        ;cx存储遍历到的ards结构体个数
    mov ebx,ards_buf        ;ebx指向缓冲区地址
    xor edx,edx             ;EDX用于保存BaseAddrLow+LengthLow最大值，此处初始化为0

.find_max_mem_area:
    mov eax,[ebx]           ;eax用于遍历缓冲区中的每一个ards的BaseAddrLow
    add eax,[ebx+8]         ;ebx+8获取的是LengthLow，故该代码计算的是BaseAddrLow+LengthLow
    add ebx,20                ;遍历下一个ards
    cmp edx,eax             ;分支语句，如果edx大于等于eax，则跳转到.next_ards，也就是进入循环
    jge .next_ards
    mov edx,eax
.next_ards:
    loop .find_max_mem_area

    mov [total_mem_bytes],edx


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

.error_hlt:
    hlt                     ;出错则挂起


[bits 32]
p_mode_start:
     
    mov ax,SELECTOR_DATA           ;初始化段寄存器，将数据段的选择子分别放入各段寄存器
    mov ds,ax
    mov es,ax                      
    mov ss,ax

    mov esp,LOADER_STACK_TOP        ;初始化栈指针，将栈指针地址放入bsp寄存器
    mov ax,SELECTOR_VIDEO           ;初始化显存段寄存器，显存段的选择子放入gs寄存器
    mov gs,ax

    mov byte [gs:160],'p'

    jmp $
