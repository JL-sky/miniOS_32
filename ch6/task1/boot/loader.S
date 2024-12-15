%include "boot.inc"
SECTION loader vstart=LOADER_BASE_ADDR

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

;------------- 定义记录ARDS结构体信息的变量 -------------
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
    add ebx,20              ;遍历下一个ards
    cmp edx,eax             ;分支语句，如果edx大于等于eax，则跳转到.next_ards，也就是进入循环
    jge .next_ards
    mov edx,eax             ;否则就是更新edx
.next_ards:
    loop .find_max_mem_area

    mov [total_mem_bytes],edx   ;将最终结果保存到total_mem_bytes


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
    mov ax,SELECTOR_DATA                    ;初始化段寄存器，将数据段的选择子分别放入各段寄存器
    mov ds,ax
    mov es,ax                      
    mov ss,ax

    mov esp,LOADER_STACK_TOP                ;初始化栈指针，将栈指针地址放入bsp寄存器
    mov ax,SELECTOR_VIDEO                   ;初始化显存段寄存器，显存段的选择子放入gs寄存器
    mov gs,ax

;------------- 加载内核 ----------------
;将内核从磁盘的9号扇区加载的内存的KERNEL_BIN_BASE_ADDR地址处
    mov eax,KERNEL_START_SECTOR
    mov ebx,KERNEL_BIN_BASE_ADDR
    mov ecx,200 
    call rd_disk_m_32

;------------- 开启分页机制 -------------
;1.构建页表和页目录表
;2.将页目录表的位置加载到控制寄存器cr3
;3.置cr0寄存器的PG位为1，打开分页机制

    call setup_page                         ;创建页目录表和页表，并初始化页内存位图
    ;以下将各段描述符的基址迁移到虚拟内存空间中                                
    mov ebx,[gdt_ptr+2]                     ;gdt_ptr+2表示GDT_BASE,也就是GDT的起始地址
    or dword [ebx+0x18+4],0xc0000000        ;ebx中保存着GDT_BASE，0x18=24，故ebx+0x18表示取出显存段的起始地址
                                            ;+4表示取出段描述符的高32位，之后和0xc0000000进行或操作
                                            ;表示将显存段的起始地址增加了3G

    add dword [gdt_ptr+2],0xc0000000        ;同理将GDT_BASE的起始地址也增加3G
    add esp,0xc0000000                      ;同理将esp栈指针的起始地址也增加3G

    mov eax,PAGE_DIR_TABLE_POS              ;将页目录表的位置加载到控制寄存器cr3
    mov cr3,eax

    mov eax,cr0                             ;打开cr0的PG位，开启分页机制
    or eax,0x80000000
    mov cr0,eax

    lgdt [gdt_ptr]                          ;开启分页后，用gdt的新地址重新加载

;------------- 进入内核函数 ----------------
enter_kernel:
    call kernel_init
    mov esp,0xc009f000
    jmp KERNEL_ENTRY_POINT

kernel_init:
    xor eax,eax
    xor ebx,ebx                             ;记录程序头表(段)地址
    xor ecx,ecx                             ;记录程序头表(段头表)中程序头数量
    xor edx,edx                             ;记录程序头表中每个条目的大小
 
    mov dx,[KERNEL_BIN_BASE_ADDR+42]        ;42字节处是e_phentsize，即程序头中每个条目的大小
    mov ebx,[KERNEL_BIN_BASE_ADDR+28]       ;28字节处是e_phoff，即程序头表的偏移
    add ebx,KERNEL_BIN_BASE_ADDR            ;程序头表的偏移加上内核在内存中的起始地址，就是程序头表的起始物理地址
    mov cx,[KERNEL_BIN_BASE_ADDR+44]        ;44字节处是e_phnum，即程序头表(段头表)中程序头数量
 
;遍历段头表，取出程序中的每一个段，将其拷贝到对应的虚拟地址空间中
.each_segment:
;检查段头表中的段头是会否是空段(PT_NULL)，如果不是就将该段拷贝到对应区域，否则就继续遍历下一个段头
    cmp byte [ebx+0],PT_NULL                ;比较p_type是否等于PT_NULL，若相等说明程序头未使用
    je .PTNULL                              ;若相等则跳转到.PTNULL
 
    ;为函数memcpy(dst,src,size)压入参数，参数依次从右向左压入，这样函数就可以从左到右取出参数
    push dword [ebx+16]                     ;实参size，程序头表偏移16字节的地方p_filesz，本段在文件内的大小
 
    mov eax,[ebx+4]                         ;程序头表偏移4字节的地方p_offset，本段在文件内的起始偏移
    add eax,KERNEL_BIN_BASE_ADDR            ;本段的偏移加上内核在内存中的起始地址，就是本段的物理地址
    push eax                                ;实参src
 
    push dword [ebx+8]                      ;实参dst，p_vaddr，本段在内存中的起始虚拟地址
 
    call mem_cpy
    add esp,12                              ;回收mem_cpy的栈帧空间
 
.PTNULL:
    add ebx,edx                             ;指向下一个段头
    loop .each_segment                      ;继续遍历段头表
    ret

;逐字节拷贝函数，将esi指向的内存区域的size个字节拷贝到edi指向的区域
mem_cpy:
    cld                                    ;指明拷贝时esi与edi的增长方向是向上的
    push ebp                               ;保存ebp
    mov ebp,esp                            ;将esp指向ebp
    push ecx                               ;rep指令会用到ecx的循环计数，而外层函数也用到了ecx的值，
                                           ;因此此处需要将外层函数的ecx的值进行备份

    mov edi,[ebp+8]                        ;参数dst
    mov esi,[ebp+12]                       ;参数src
    mov ecx,[ebp+16]                       ;参数size
    rep movsb                              ;rep(repeat)指令，重复执行movsb指令
                                           ;movsb指令，s表示string，b表示byte，
                                           ;即将esi指向的内存拷贝一个字节给edi指向的内存
                                           ;因此本条指令表示逐字节拷贝，拷贝的字节个数为ecx的值
 
    pop ecx                                ;取出备份的值
    pop ebp                                ;返回上层函数
    ret

;------------- 创建页目录表和页表 -------------
;初始化页目录表和页表

;逐字节清空页目录表
setup_page:
    mov ecx,4096                                    ;页目录表的大小为4KB，ecx是loop指令的循环计数器
                                                    ;由于初始化页表是逐字节置0的，因此ecx的值为4096
    mov esi,0                                       ;页目录表的偏移量
.clear_page_dir:
    mov byte [PAGE_DIR_TABLE_POS+esi],0             ;逐字节清空页目录表
                                                    ;其中PAGE_DIR_TABLE_POS为页目录表初始地址的宏
    inc esi                                         ;递增偏移量，清空下一个字节
    loop .clear_page_dir

;初始化创建页目录表
.create_pde:
    mov eax,PAGE_DIR_TABLE_POS                      ;eax保存页目录表的起始地址
    add eax,0x1000                                  ;0x1000为1k，故该代码的计算结果是将eax指向第一张页表的起始地址
    mov ebx,eax                                     ;ebx保存第一张页表的起始地址，后续会用到
    or eax,PG_US_U|PG_RW_W|PG_P                     ;eax已经有了第一张页表的起始地址
                                                    ;此处再加上属性，即可表示为页目录表的一个表项，
                                                    ;该表项代表的是第一张页表的物理地址及其相关属性

    mov [PAGE_DIR_TABLE_POS+0x0],eax                ;页目录表的第一个表项指向第一张页表
    mov [PAGE_DIR_TABLE_POS+0xc00],eax              ;0xc0000000即为3GB，由于我们划分的虚拟地址空间3GB以上为os地址空间
                                                    ;因此该语句是将3GB的虚拟空间映射到内核空间 
                                                    ;而0xc00/4=768，也就是说页目录表的768号表项映射为物理内核空间
    
    sub eax,0x1000          
    mov [PAGE_DIR_TABLE_POS+4092],eax               ;最后一个页表项指向自己，为将来动态操作页表做准备


;创建第一张页表的页表项，由于os的物理内存不会超过1M，故页表项个数的最大值为1M/4k=256
    mov ecx,256                                     ;循环计数器
    mov esi,0                                       ;偏移量
    xor edx,edx                                     ;第一个页表项指向物理空间的0~4KB物理页框
                                                    ;故记录的起始地址就是第一个页框的起始地址，也就是0
    mov edx,PG_US_S|PG_RW_W|PG_P                    ;此时的edx表示拥有属性PG_US_S|PG_RW_W|PG_P
                                                    ;且物理地址为0的物理页的页表项
.create_pte:
    mov [ebx+esi*4],edx                             ;此前ebx已经保存了第一张页表的起始地址
    add edx,4096                                    ;edx指向下一个物理页（一个物理页4KB）
    inc esi                                         ;esi指向页表的下一个偏移
    loop .create_pte

; -------------------初始化页目录表769号-1022号项，769号项指向第二个页表的地址（此页表紧挨着上面的第一个页表），770号指向第三个，以此类推
    mov eax,PAGE_DIR_TABLE_POS
    add eax,0x2000                                  ;此时的eax表示第二张页表的起始地址
    or eax,PG_US_U|PG_RW_W|PG_P                     ;为eax表项添加属性

    mov ebx,PAGE_DIR_TABLE_POS
    mov ecx,254                                     ;要设置254个页表项
    mov esi,769                                     ;从第769个页表项开始设置
.create_kernel_pde:
    mov [ebx+esi*4],eax                             ; 设置页目录表项
    inc esi                                         ; 增加要设置的页目录表项的偏移
    add eax,0x1000                                  ; eax指向下一个页表的位置，由于之前设定了属性，所以eax是一个完整的指向下一个页表的页目录表项
    loop .create_kernel_pde                         ; 循环设定254个页目录表项
    ret


; ------ 32位模式下从磁盘中加载文件到内存 ------
rd_disk_m_32:
    ;参数：
        ; eax=LBA扇区号
        ; ebx=将数据写入的内存地址
        ; ecx=读入的扇区数
    mov esi,eax	                                        ;备份eax
    mov di,cx		                                    ;备份cx
                                                        ;读写硬盘:
                                                        ;第1步：选择特定通道的寄存器，设置要读取的扇区数
    mov dx,0x1f2
    mov al,cl
    out dx,al                                           ;读取的扇区数
 
    mov eax,esi	                                        ;恢复ax
 
                                                        ;第2步：在特定通道寄存器中放入要读取扇区的地址，将LBA地址存入0x1f3 ~ 0x1f6
                                                        ;LBA地址7~0位写入端口0x1f3
    mov dx,0x1f3                       
    out dx,al                          
 
                                                        ;LBA地址15~8位写入端口0x1f4
    mov cl,8
    shr eax,cl
    mov dx,0x1f4
    out dx,al
 
                                                        ;LBA地址23~16位写入端口0x1f5
    shr eax,cl
    mov dx,0x1f5
    out dx,al
 
    shr eax,cl
    and al,0x0f	                                        ;lba第24~27位
    or al,0xe0	                                        ; 设置7～4位为1110,表示lba模式
    mov dx,0x1f6
    out dx,al
 
                                                        ;第3步：向0x1f7端口写入读命令，0x20 
    mov dx,0x1f7
    mov al,0x20                        
    out dx,al
 
                                                        ;第4步：检测硬盘状态
.not_ready:
                                                        ;同一端口，写时表示写入命令字，读时表示读入硬盘状态
    nop
    in al,dx
    and al,0x88	                                        ;第4位为1表示硬盘控制器已准备好数据传输，第7位为1表示硬盘忙
    cmp al,0x08
    jnz .not_ready	                                    ;若未准备好，继续等。
 
                                                        ;第5步：从0x1f0端口读数据
    mov ax, di                                          ;di当中存储的是要读取的扇区数
    mov dx, 256                                         ;每个扇区512字节，一次读取两个字节，所以一个扇区就要读取256次，与扇区数相乘，就等得到总读取次数
    mul dx                                              ;8位乘法与16位乘法知识查看书p133,注意：16位乘法会改变dx的值！！！！
    mov cx, ax	                                        ; 得到了要读取的总次数，然后将这个数字放入cx中
    mov dx, 0x1f0
.go_on_read:
    in ax,dx
    mov [ebx],ax                                        ;与rd_disk_m_16相比，就是把这两句的bx改成了ebx
    add ebx,2		        
                                                        ; 由于在实模式下偏移地址为16位,所以用bx只会访问到0~FFFFh的偏移。
                                                        ; loader的栈指针为0x900,bx为指向的数据输出缓冲区,且为16位，
                                                        ; 超过0xffff后,bx部分会从0开始,所以当要读取的扇区数过大,待写入的地址超过bx的范围时，
                                                        ; 从硬盘上读出的数据会把0x0000~0xffff的覆盖，
                                                        ; 造成栈被破坏,所以ret返回时,返回地址被破坏了,已经不是之前正确的地址,
                                                        ; 故程序出会错,不知道会跑到哪里去。
                                                        ; 所以改为ebx代替bx指向缓冲区,这样生成的机器码前面会有0x66和0x67来反转。
                                                        ; 0X66用于反转默认的操作数大小! 0X67用于反转默认的寻址方式.
                                                        ; cpu处于16位模式时,会理所当然的认为操作数和寻址都是16位,处于32位模式时,
                                                        ; 也会认为要执行的指令是32位.
                                                        ; 当我们在其中任意模式下用了另外模式的寻址方式或操作数大小(姑且认为16位模式用16位字节操作数，
                                                        ; 32位模式下用32字节的操作数)时,编译器会在指令前帮我们加上0x66或0x67，
                                                        ; 临时改变当前cpu模式到另外的模式下.
                                                        ; 假设当前运行在16位模式,遇到0X66时,操作数大小变为32位.
                                                        ; 假设当前运行在32位模式,遇到0X66时,操作数大小变为16位.
                                                        ; 假设当前运行在16位模式,遇到0X67时,寻址方式变为32位寻址
                                                        ; 假设当前运行在32位模式,遇到0X67时,寻址方式变为16位寻址.
    loop .go_on_read
    ret
