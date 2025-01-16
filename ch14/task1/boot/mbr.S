;主引导程序MBR，由BIOS通过jmp 0:0x7c00跳转

;------------------------------------------
%include "boot.inc"
;vstart=0x7c00表示本程序在编译时，起始地址编译为0x7c00
SECTION MBR vstart=0x7c00
;使用通用寄存器中的值（0）初始化其余寄存器
    mov ax,cs
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov fs,ax
;初始化栈指针
    mov sp,0x7c00
;存入显存的段基址
    mov ax,0xb800
    mov gs,ax
;------------------------------------------


;------------------------------------------
;利用0x06号功能进行清屏
;INT 0x10 功能号：0x06 功能描述：上卷窗口清屏
;输入:
    ;AH 功能号：0x06
    ;AL=上卷的行数（如果为0，则表示全部）
    ;BH=上卷的行属性
    ;(CL,CH)=窗口左上角(X,Y)位置
    ;(DL,DH)=窗口右下角(X,Y)位置
;返回值
    ;无返回值
    mov ax,0x0600
    mov bx,0x0700
    mov cx,0        ;左上角(0,0)
    mov dx,0x184f   ;右下角(80,25)
                    ;0x18=24，0x4f=79
    int 0x10        ;调用BIOS中断函数
;------------------------------------------

;------------------------------------------
;将要显示的字符串写入到显存中
    mov byte [gs:0x00],'1';在第一个字节的位置写入要显示的字符“1”
    ;在第二个字节的位置写入显示字符（也就是字符1）的属性，其中A表示绿色背景，4表示前景色为红色
    mov byte [gs:0x01],0xA4

    mov byte [gs:0x02],' '
    mov byte [gs:0x03],0xA4
    
    mov byte [gs:0x04],'M'
    mov byte [gs:0x05],0xA4
    
    mov byte [gs:0x06],'B'
    mov byte [gs:0x07],0xA4

    mov byte [gs:0x08],'R'
    mov byte [gs:0x09],0xA4

    mov eax,LOADER_START_SECTOR;起始扇区地址
    mov bx,LOADER_BASE_ADDR;要写入的内存地址
    mov cx,4;待读入的扇区数目
    call rd_disk_m16;调用磁盘读取程序

    jmp LOADER_BASE_ADDR+0x300;

;------------------------------------------
;函数功能：读取硬盘的n个扇区
;参数：
    ;eax：LBA扇区号
    ;bx：要写入的内存地址
    ;cx：待读入的扇区数目

rd_disk_m16:
;out命令，通过端口向外围设备发送数据
;其中目的操作数可以是8为立即数或者寄存器DX,源操作数必须是寄存器AL或者AX
;因此需要将之前ax中保存的值进行备份
    mov esi,eax
    mov di,cx

;正式读写硬盘
;第一步：设置要读取的扇区数目
    mov dx,0x1f2
    mov ax,cx
    out dx,ax

    mov eax,esi;恢复eax的值

;第二步：将LBA的地址存入0x1f3~0x1f6
    ;LBA的0~7位写入端口0x1f3
    mov dx,0x1f3
    out dx,al

    ;LBA的8~15位写入端口0x1f4
    mov cl,8
    shr eax,cl;ax左移8位
    mov dx,0x1f4
    out dx,al

    ;LBA的16~23位写入端口0x1f5
    shr eax,cl
    mov dx,0x1f5
    out dx,al

    ;LBA的24~27位写入端口0x1f6
    shr eax,cl;左移8位
    and al,0x0f;与0000 1111相与，取后四位
    or al,0xe0;设置7~4位为1110，表示lba模式
    mov dx,0x1f6
    out dx,al

;第三步，向0x1f7端口写入读命令，0x20
    mov dx,0x1f7
    mov al,0x20
    out dx,al

;第四步，检测硬盘状态
    .not_ready:
        nop;空操作，什么也不做，相当于sleep，只是为了增加延迟
        in al,dx;由于读取硬盘状态信息的端口仍旧是0x1f7，因此不需要再为dx指定端口号
        
        ;取出状态位的第3位和第7位
        ;其中第3位若为1表示硬盘控制器已经准备好数据传输,第7位为1表示硬盘忙
        and al,0x88
        ;cmp为减法指令，此处是为了判断第7位是否为1，如果为1，则减法的结果应该为0
        cmp al,0x08
        ;JNZ是条件跳转指令，它表示"Jump if Not Zero"，
        ;也就是如果零标志位（ZF）不为0，则进行跳转。否则不进行跳转
        jnz .not_ready;若未准备好，则继续等
    
;第五步，从0x1f0端口读取数据
    mov ax,di;di是之前备份的要读取的扇区数
    mov dx,256
    ;mul指令表示乘法操作，当只有一个操作数时，被乘数隐含在al或者ax中
    mul dx;一个扇区512字节，每次读取两个字节，需要读取di*256次
    mov cx,ax;cx在此时表示要循环读取的次数

    mov dx,0x1f0
    .go_on_read:
        in ax,dx
        mov [bx],ax;通过循环将输入写入bx寄存器所指向的内存，每次读入2个字节的数据
        add bx,2
        loop .go_on_read
        ret

    times 510-($-$$) db 0
    db 0x55,0xaa
