# miniOS_32
本仓库参考《操作系统真象还原》手写一个32位的操作系统内核

## 启动

### 启动过程

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740726468552-d5f30c25-09d3-435c-972c-8907346e6c0e.png)

- `启动过程`，BIOS->MBR->Bootloader->OS，多 CPU 情况下 BSP 启动 APs，叙述每个阶段做的主要事情。
- `上电那一刻，CS:IP = 0xf000:0xfff0，此地址上的 16 字节是个跳转地址：jmp f000:e05b（据然还真问到了具体跳转地址）`

### BIOS

- - **做的事情：**自检程序，将 MBR 加载到 0x7c00
  - `提问其他的固件方面的知识，BIOS，UEFI等，不太了解`

### MBR

- - `MBR 构成`，引导程序->64字节分区表->魔数(0x55和0xAA)
  - `在哪儿`，启动盘最开始那个扇区
  - `加载到哪儿`，加载到 **0x7c00**
  - `主要干的事情`，根据分区表找到一个活动分区，然后加载 Bootloader
  - `如何判断是否是启动盘`，魔数 0x55 和 0xAA

### loader

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740576183606-fc8c4216-2c9d-4b97-bd1c-c7c80815657b.png)

- - **磁盘位置**：loader程序的磁盘位置在**第0块磁盘的第2扇区**，
  - **加载位置：**内存中的加载地址为**0x900**
  - **功能：**

- - - **开启保护模式（构建GDT）**
    - **开启分页机制（构建页表和页目录表）**
    - **加载内核文件**

- *BSP 启动 AP 过程*(x86，中断控制器为APIC)，主要通过 LAPIC 发送 INIT-SIPI-SIPI 消息... 详见 Multiprocessor Specification

#### 开启保护模式

##### 实模式和保护模式的区别，为什么要有保护模式

| **比较项目** | **实模式**                                                   | **保护模式**                                                 |
| ------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
| 内存寻址方式 | 采用段基址左移 4 位加上段内偏移地址的方式形成 20 位物理地址，最大寻址空间为 1MB | 采用虚拟地址空间，通过段描述符表（如 GDT 全局描述符表、LDT 局部描述符表）进行地址转换，可寻址 4GB 内存 |
| 内存保护机制 | 几乎**没有内存保护机制**，任何程序都可以访问任何内存地址，容易导致程序间的相互干扰和破坏 | 具有完善的内存保护机制，通过**段描述符中的权限位（如访问权限、特权级等）来控制对内存的访问**，不同特权级的程序只能访问特定的内存区域，保护系统和其他程序的安全 |
| CPU 特权级别 | 只有一种特权级别，所有程序都具有相同的权限，**操作系统和应用程序处于同一特权级别，操作系统容易受到应用程序的破坏** | 支持 4 种特权级别（0 - 3 级），**通常操作系统内核运行在 0 级，具有最高权限；应用程序运行在 3 级，权限最低。不同特权级之间的转换有严格的控制，提高了系统的稳定性和安全性** |
| 中断处理     | 中断向量表位于内存的 0 地址开始的 1KB 空间（0x00000 - 0x003FF），每个中断向量占用 4 个字节，指向中断处理程序的入口地址 | 采用中断描述符表（IDT），每个中断描述符占用 8 个字节，包含了中断处理程序的入口地址、特权级等信息，**对中断处理的管理更加灵活和安全** |
| 多任务的支持 | 不支持真正意义上的多任务处理，一次只能运行一个程序，程序之间的切换需要人工干预 | 支持多任务处理，**通过内存保护机制和特权级别控制，不同任务之间相互隔离，提高了系统的并发处理能力和稳定性** |



![img](https://cdn.nlark.com/yuque/0/2025/jpeg/53299329/1740539239176-d468d606-b946-4e41-b4a1-cc5c92ba0910.jpeg)

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740540392449-6fb12e95-1db5-4166-a0a0-2407f66cc726.png)

**段描述符**

![img](https://cdn.nlark.com/yuque/0/2025/jpeg/53299329/1740539313238-6689844b-93d5-4864-952f-768d1042e7a4.jpeg)![img](https://cdn.nlark.com/yuque/0/2025/jpeg/53299329/1740539323897-c2a30bf3-0f06-4eb4-a808-cd67bcd9c21d.jpeg)

**选择子**

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740876493021-1f5181b7-ac20-4286-abcf-5299820f777a.png)

**控制寄存器CR0**

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740876501067-8226f5e9-10e6-44c8-a44e-e8e6283ef993.png)

##### 如何开启保护模式？

[《操作系统真象还原》第四章——保护模式入门](https://jl-sky.github.io/p/操作系统真象还原第四章保护模式入门/)

- 打开A20
- 加载GDT

- - 在实模式下**构造需要的段描述符**（代码段、数据段、栈段、显存段）
  - 将四种段描述符装填进GDT，从而**构造GDT**
  - **构造四种内存段的段选择子**
  - **构造GDTR**（存储GDT基址的寄存器），从而让操作系统运行时将GDT加载进内存

- **置cr0寄存器的PE位为1**

```cpp
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
```

**保护模式下的基本使用**

```cpp
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
```

#### 如何开启分页机制？

- 构建页表和页目录表
- 将页目录表的位置**加载到控制寄存器cr3**
- 置**cr0寄存器的PG位**为1，打开分页机制

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740876518047-5ceff6ce-26ad-417c-a35f-4c8530ab58ed.png)



**页目录项和页表项**

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740876590910-816393da-c586-4dea-be66-fd0ac2ea5687.png)

**控制寄存器cr3**

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740876543308-32d1674e-d965-4060-b18d-6b423b725d57.png)



##### 页表与页目录表的关系与内存布局

- 4GB虚拟地址空间，一页内存占4KB，故共有1M个页面，也就是1M个页表项
- 每个页表项占4B，故所有页表项共占用4MB
- 将这4MB的页表项重新划分页面，则4MB/4KB=1024个页目录项



页目录表：一张，1024个页目录项

页表：1024张，每张页表1024个页表项

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740876600829-5e0edd7c-2a54-4176-b5d8-9d3de5331440.png)



##### 地址划分

- 物理内核空间：低端1MB
- 一级页表：由于一张页表可表示1024*4KB=4MB的物理空间，因此可以用0号页表映射物理内核空间
- 二级页表：由于3GB~4GB这段虚拟空间映射内核空间，故而让二级页表的768号页表项（代表着虚拟地址的3GB其实地址）映射到一级页表的0号页表
- 内核空间共享：由于768号页目录项到1023号页目录项这段虚拟内存映射的是内核物理空间，因此此后进程的空间只需复制这段页目录项即可

![img](https://raw.githubusercontent.com/jl-sky/imageDatesets/master/2024/12/upgit_20241220_1734690932.png)

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740572308914-f7964bae-bdf4-408d-a50e-9cfa40afc337.png)

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740572328445-df57032c-e12e-49c1-a43a-5b35ac53dfb9.png)



#### 加载内核

程序文件（内核文件）存放在磁盘，文件的内容可分为两部分

- **程序头（处理器不可执行）**：elf header，记录了程序的入口地址，段头表（记录段的基本信息）、节头表（记录节的基本信息）
- **程序体（处理器真正可执行）**：处理器可以执行的代码和数据

因此内核加载主要分为两步

1. 将内核文件从磁盘加载到内存
2. 读取elf header，获取段信息，然后抽取文件中的代码段和数据，作为内核映射，放到内存的某个位置（0x1500），让处理器去执行代码

实际应用如下

```cpp
gcc -c main.c  # 编译生成目标文件 main.o
ld -Ttext 0x00001500 -e main main.o -o main.out  # 链接生成可执行文件 main.out
```

上述代码生成的生成的可执行文件 `main.out` 的**代码段将从地址** `**0x00001500**` **开始加载，并且程序将从** `**main**` **函数开始执行。**

以下是**抽取内核映像**的步骤

1. 从elf头中读取程序头表的信息

- - 程序头表的初始偏移
  - 程序头表中条目（记录段的信息）的数量
  - 程序头表中每个条目的大小

1. 读取到程序头表的信息后，我们就可以像遍历数组一样遍历程序头表，取出程序头表中的每个程序头（也就是段头）的信息

- - 本段在文件内的大小
  - 本段在文件内的起始偏移
  - 本段在内存中的起始虚拟地址

1. 将段复制到内存指定的虚拟地址处



### `汇编和 C 交互的一些问题`

#### 使用调用约定（文件互相调用）

##### 参数传递

![img](https://cdn.nlark.com/yuque/0/2025/jpeg/53299329/1740637132254-d07b4b98-09f2-4327-8ba8-2bc6134010d3.jpeg)

**现代编译器的参数传递方式**

- 如果参数的个数小于7个，则采用寄存器传递，因为寄存器传递参数速度较快。这7个寄存器分别是

- - rdi，传递第一个参数
  - rsi，传递第二个参数
  - rdx，传递第三个参数
  - rcx，传递第四个参数
  - r8，传递第五个参数
  - r9，传递第六个参数

- 当参数个数大于等于7个，多余的参数则使用栈空间传递

**项目中使用的参数传递约定**

- ebx存储第一个参数
- ecx存储第二个参数
- edx存储第三个参数
- esi存储第四个参数
- edi存储第五个参数

**参数传递顺序**

- **调用者**按照**逆序**存储参数
- **被调用者顺序**取出参数

##### 案例说明

```cpp
int subtract(int a,int b);  //被调用者
int sub=subtract(3,2);      //调用者
```

以下是调用者的汇编代码

```cpp
;调用者
push 2          ;压入参数b
push 3          ;压入参数a
call subtract   ;调用subtract函数
add esp,8       ;回收栈空间
```

以下是被调用者的汇编代码

```cpp
;被调用者，也就是subtract
subtract:
    push ebp
    mov ebp,esp

    mov eax,[ebp+8]     ;取出第一个参数a
    add eax,[ebp+0xc]   ;取出第二个参数b

    pop ebp
    ret
```

![img](https://cdn.nlark.com/yuque/0/2025/jpeg/53299329/1740637393131-41f1c382-fd7e-4c98-89ee-69c4cf240fdf.jpeg)

需要知道，call指令在调用函数时做了两件事

- 将call指令的下一条指令的地址压入栈中，作为函数的返回地址
- 修改eip寄存器（指令寄存器，用于取出指令送给cpu执行）的值为subtract符号的地址

相对的，ret指令也做了两件事

- 弹出调用者压在栈中的返回地址
- 修改eip寄存器的值为弹出的返回地址

##### c与汇编的混合调用

![img](https://cdn.nlark.com/yuque/0/2025/jpeg/53299329/1740644322098-362a461b-98e9-4637-97d8-90100329851a.jpeg)

```cpp
extern void asm_print(char*,int);
void c_print(char* str){
    int len=0;
    while(str[len++]);
    asm_print(str,len);
}
```



```cpp
section .data
str: db "asm print says:hello wolrd!",0xa,0
str_len equ $ - str

section .text
extern c_print
global _start
_start:
    push str
    call c_print
    add esp,4

    mov eax,1
    int 0x80

global asm_print
asm_print:
    push ebp
    mov ebp,esp

    mov eax,4
    mov ebx,1
    mov ecx,[ebp+8]
    mov edx,[ebp+12]
    int 0x80

    pop ebp
    ret
nasm -f elf32 ./C_with_S_S.S -o C_with_S_S.o
gcc-4.4 -m32 -c ./C_with_S_c.c -o C_with_S_c.o
ld -m elf_i386 ./C_with_S_c.o C_with_S_S.o -o main
```



### 内联汇编

### `OS 第一个 init 进程做了些什么`

- **中断**初始化

- - 初始化并加载中断描述符表（包括系统调用）
  - 时钟中断初始化
  - 键盘中断初始化

- **系统调用**初始化

- - 注册并初始化常见的系统调用

- **内存**初始化

- - 初始化物理内存池

- - - 初始化**内核物理内存池**

- - - - 内核物理内存池起始地址

- - - - - 页目录表和页表的上方

- - - - 管理内核物理内存池的位图结构

- - - - - 位图长度
        - 位图的起始地址（0xc009a000）

- - - 初始化**用户物理内存池**（同内核物理内存池）

- - 初始化虚拟内存池

- - - 初始化**内核虚拟内存池**

- - - - 内核虚拟内存池的起始地址

- - - - - 内核堆空间的起始地址（0xc0100000）

- - - - 初始化管理内核虚拟内存池的位图结构

- 线程初始化

- - 内核main线程的初始化

- - - 初始化main线程的pcb
    - 将main线程的pcb插入到all_list中



- xv6 里面打开 0 1 2 号文件
- fork 出 shell
- 然后 wait（等待孤儿进程过继给init）
- `提问现在的 Linux 里面干了些什么`，有简单看了看，有些复杂待研究

## 中断管理

### 中断分类及介绍

![img](https://cdn.nlark.com/yuque/0/2025/jpeg/53299329/1740617073167-dbde85e7-f9a1-4cc4-95d0-8cb67cfdd3c8.jpeg)

**外部中断的信号线**

![img](https://raw.githubusercontent.com/jl-sky/imageDatesets/master/2024/12/upgit_20241217_1734423679.png)

### 中断向量、中断描述符、中断描述符表

**中断向量**

**中断机制本质上是来了一个中断信号后，调用相应的中断处理程序进行处理，所以CPU为了统一中断管理，就为每个中断信号分配一个整数，用此整数作为中断的ID，这个整数就是所谓的中断向量，然后用此ID作为中断描述符表中的索引，然后找到对应的表项，进而即可从中找到对应的中断处理程序。**

其中

- **异常和不可屏蔽**的中断向量号由**CPU**自动提供
- **可屏蔽中断**的中断向量号由**中断代理**（8259A）提供
- **软中断**的中断向量号由**软件**提供

![img](https://raw.githubusercontent.com/jl-sky/imageDatesets/master/2024/12/upgit_20241217_1734426724.png)

![img](https://cdn.nlark.com/yuque/0/2025/jpeg/53299329/1740618552812-53c3609b-e1f6-48ed-af13-818f5e5a7ad3.jpeg)

### 中断处理过程

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740620469203-366952e7-2ebd-4618-9934-b22b8ef9f886.png)

![img](https://cdn.nlark.com/yuque/0/2025/webp/53299329/1740621014570-a8b70975-10a2-4fe3-b710-5b75399c599f.webp)

#### 中断控制器干了什么事

中断控制器**主要负责管理和协调外部设备向 CPU 发出的中断请求**

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740622098946-7834413c-eed3-4c3a-8d20-348f2e461c67.png)

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740622077623-7ee3a3f5-05c9-4f7a-8643-bd3b97f24675.png)

1. **中断信号接收**：外设发出中断信号，经主板信号通路送至 8259A 芯片的特定 IRQ 接口。
2. **中断屏蔽检查**：8259A 检查 IMR 寄存器中是否已经屏蔽了来自该IRO接口的中断信号

1. 1. 若对应 IRQ 接口位为 1，中断信号被屏蔽丢弃；
   2. 若为 0，信号送入 IRR 寄存器，对应位设为 1，然后将信号送入IRR寄存器，IRR 相当于待处理中断队列。

1. **优先级仲裁**：在合适时机，优先级仲裁器 PR 从 IRR 中挑选优先级最高的中断（IRQ 接口号越低，优先级越高，IRQ0 最高）。
2. **向 CPU 发送中断请求**：8259A 通过 INT 接口向 CPU 的 INTR 接口发送 INTR 信号，告知有新中断。
3. **CPU 响应中断**：CPU 执行完当前指令后，通过 INTA 接口向 8259A 回复中断响应信号，表示准备好处理。
4. **更新中断状态**：8259A 收到响应信号后，将选出的最高优先级中断在 ISR 寄存器对应位置 1（表示正在处理），同时将 IRR 寄存器中该中断对应位置 0（从待处理队列移除）。
5. **传递中断向量号**：CPU 再次发送 INTA 信号向 8259A 请求中断向量号，8259A 用起始中断向量号加 IRQ 接口号得到该设备的中断向量号，并通过系统数据总线发送给 CPU。
6. **执行中断处理程序**：CPU 获取中断向量号后，将其作为索引，在中断向量表或中断描述符表中找到相应的中断处理程序并执行。



#### `如何定位的中断服务程序`

- x86 的情况是硬件识别，向量中断的方式

- - 根据中断号在中断描述符表里**找到****中断描述符**，**中断描述符里存着中断程序的****段选择子****和****入口地址**
  - cpu根据段选择子在GDT中寻找对应的**代码段基址**，加上**中断程序的偏移量**，然后去执行中断程序

![img](https://cdn.nlark.com/yuque/0/2025/jpeg/53299329/1740618708747-b8d648e8-47c6-4beb-9713-418f5b4f7720.jpeg)

- 还有软件识别的方式，跳到一个固定地址，然后再查询异常状态寄存器看是什么异常，再跳到特定的处理程序，一般精简指令集这么干。

#### `开关中断的事`

- 一般我们说中断时保存上下文前要关中断，x86 下，关中断就是 EFLAGS 的 IF = 0

#### `系统调用过程`

![img](https://cdn.nlark.com/yuque/0/2025/jpeg/53299329/1740637992604-b1c80f2a-a197-4c12-ab9b-5d136a20b431.jpeg)



- 构建系统调用所需的中断描述符
- 构建触发系统调用中断的转接口，该转接口的作用是**将eax中的系统调用号作为索引，然后按照索引寻找syscall_table中对应的系统调用例程**

##### printf的实现

```cpp
/* 格式化输出字符串format */
uint32_t printf(const char *format, ...)
{
    va_list args;
    va_start(args, format); // 使args指向format
    char buf[1024] = {0};   // 用于存储拼接后的字符串
    vsprintf(buf, format, args);
    va_end(args);
    return write(buf);
}
/* 将参数ap按照格式format输出到字符串str,并返回替换后str长度 */
uint32_t vsprintf(char *str, const char *format, va_list ap)
{
    char *buf_ptr = str;
    const char *index_ptr = format;
    char index_char = *index_ptr;
    int32_t arg_int;
    char *arg_str;
    while (index_char)
    {
        if (index_char != '%')
        {
            *(buf_ptr++) = index_char;
            index_char = *(++index_ptr);
            continue;
        }
        index_char = *(++index_ptr); // 得到%后面的字符
        switch (index_char)
        {
        case 's':
            arg_str = va_arg(ap, char *);
            strcpy(buf_ptr, arg_str);
            buf_ptr += strlen(arg_str);
            index_char = *(++index_ptr);
            break;
        case 'c':
            *(buf_ptr++) = va_arg(ap, char);
            index_char = *(++index_ptr);
            break;
        case 'd':
            arg_int = va_arg(ap, int);
            if (arg_int < 0)
            {
                arg_int = 0 - arg_int; /* 若是负数, 将其转为正数后,再正数前面输出个负号'-'. */
                *buf_ptr++ = '-';
            }
            itoa(arg_int, &buf_ptr, 10);
            index_char = *(++index_ptr);
            break;
        case 'x':
            arg_int = va_arg(ap, int);
            itoa(arg_int, &buf_ptr, 16);
            index_char = *(++index_ptr); // 跳过格式字符并更新index_char
            break;
        }
    }
    return strlen(str);
}
```

| 栈地址 |            | 内容         |
| ------ | ---------- | ------------ |
| 高地址 | 第二个变参 | "test"       |
|        | 第一个变参 | 10           |
|        | format     | "a=%d, b=%s" |
| 低地址 |            | 返回地址     |

- 初始时，**args（即vsprintf的参数ap）指向format的地址**
- **逐个字符判断format的字符（即vsprintf的第二个参数format）是否是"%"**，如果不是就将字符拷贝给str（即vsprintf的第二个参数format）
- 如果发现字符%，就将指向format的字符后移，判断下一个字符是x、s、c、d中哪一个，然后匹配进去
- 匹配进去之后，**参数ap的地址后移，取出第一个变参**，然后**对变参进行处理（比如如果匹配的是s，就将变参直接拷贝，如是x，就将变参转换为十六进制）**

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740834435243-794446ac-20e0-4f28-a7e2-f66540ee5986.png)

- - 没有中断控制器这个过程，其他阶段基本相同
  - 传参两种方式，寄存器，压栈(进入内核后从上下文中的栈指针寄存器获取用户态栈顶地址)
  - **x86下系统调用可以用中断门实现**，也可以使用陷阱门实现，**使用中断门进入中断自动关中断，如果使用陷阱门则不会自动关中断，这是二者唯一的区别**





### 

### `DMA 过程`

- 不是很清楚，百度google，我也是搜的。

### `磁盘寻址`

- CHS(柱面磁头扇区)、LBA(逻辑块地址)，实际上似乎不是想问这个，但面试官当时也没说清楚就下一个问题，emm??????



### `内核态、用户态的理解`

实际上就是特权级，***RPL、CPL、DPL\*** *三者之间不同情况下的各种比较变化*，特复杂。

- **CPL（当前特权级别）**：表示当前执行代码的特权级别，存储在 CS 和 SS 低 2 位。
- **DPL（描述符特权级别）**：**段描述符或门描述符**中的特权级别，指定访问该段或门所需的最低特权级别。
- **RPL（请求特权级别）**：**段选择子**低 2 位的特权级别，CPU 取 CPL 和 RPL 中的较低值作为有效特权级与 DPL 比较，只有有效特权级小于等于 DPL 时，才能进行访问或调用。



- “完整的程序=用户代码+内核代码”。而**这个完整的程序就是我们所说的任务，也就是线程或进程**
- 任务在执行过程中会执行用户代码和内核代码
- 当处理器处于低特权级下执行用户代码时我们称之为用户态，当处理器进入高特权级执行到内核代码时我们称之为内核态
- 当处理器从用户代码所在的低特权级过渡到内核代码所在的高特权级时，这称为陷入内核。
- 无论是执行用户代码，还是执行内核代码，这些代码都属于这个完整的程序，即属于当前任务，并不是说当前任务由用户态进入内核态后当前任务就切换成内核了，这样理解是不对的。



### 中断发生时的压栈

- 任务执行中特权级变换本质是处理器当前特权级改变，这涉及栈的使用问题。
- 为避免栈混乱和溢出，**处理器在不同特权级下需使用对应特权级的栈**，如 0 特权级用 0 特权级的栈，3 特权级用 3 特权级的栈。



- 当发生**从低特权级（如用户态，通常特权级为 3）向高特权级**（如内核态，通常特权级为 0）的转换时，需要切换到高特权级对应的栈。通常会**从 TSS（任务状态段）中加载高特权级的栈段寄存器** `**SS**` **和栈指针寄存器** `**ESP**`（在 64 位系统中是 `RSP`）的值，从而切换到高特权级的栈进行后续操作。
- 同时，为了在**高特权级操作完成后能够正确返回到低特权级**的执行环境，**需要把低特权级的** `**SS**` **和** `**ESP**` **压入高特权级的栈中保存起来**。这样在返回低特权级时，可以从栈中恢复低特权级的 `SS` 和 `ESP` 值，从而切换回低特权级的栈继续执行。

#### 无特权级切换的情况

#### ![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740748120653-5c4e69dd-820a-4a35-9b01-6a20900d9019.png)

当发生中断时，如果当前执行程序的特权级和中断服务程序的特权级相同（即没有特权级切换），CPU 会按以下顺序将信息压入栈中：

1. **EFLAGS 寄存器**：保存当前的标志寄存器的值，包括进位标志、零标志等，这些标志反映了 CPU 执行上一条指令后的状态。
2. **CS 寄存器**：保存当前代码段寄存器的值，用于记录中断发生时程序所在的代码段。
3. **EIP 寄存器**：保存当前指令指针寄存器的值，用于记录中断发生时程序执行到的位置。

#### 有特权级切换的情况

#### ![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740748108630-4fe2b8f7-2507-4e04-8142-fe6eaa522dc8.png)

当发生中断时，如果当前执行程序的特权级和中断服务程序的特权级不同（通常是从用户态切换到内核态），CPU 会按以下顺序将更多信息压入栈中：

1. **SS 寄存器（新特权级的栈段寄存器）**：保存新特权级（通常是内核态）的栈段寄存器的值，用于指定新的栈段。
2. **ESP 寄存器（新特权级的栈指针寄存器）**：保存新特权级的栈指针寄存器的值，用于指定新栈的栈顶位置。
3. **EFLAGS 寄存器**：同无特权级切换时一样，保存当前的标志寄存器的值。
4. **CS 寄存器**：保存当前代码段寄存器的值。
5. **EIP 寄存器**：保存当前指令指针寄存器的值。
6. **错误码（如果中断产生了错误码）**：某些中断会产生错误码，用于指示中断发生的原因。



#### `用户态到内核态栈的变化`

- *x86 下根据 TR 可以找到 TSS，TSS 里面有内核栈的 CS 和 ESP；RISC-V 下 sscratch 有内核上下文的地址*。

## 内存管理

![img](https://cdn.nlark.com/yuque/0/2025/webp/53299329/1740644670370-780d6411-b744-4544-a60a-f63ac3afc079.webp)

- `寻址方式`，x86 段基址：段内偏移

- - 实模式下段寄存器为实际的段基址，保护模式下为段选择子

- `地址转换过程`，段级转换(GDT)、页级转换(查页表)
- GDTR 中存放 GDT 的线性地址，CR3 里面存放页目录的物理地址，页表项里面存放的也是物理地址。

### `虚拟地址空间如何布局的`  ![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740665544386-1377c9f8-8b6e-4ede-861c-e33cbef9aeda.png)![img](https://cdn.nlark.com/yuque/0/2025/webp/53299329/1740644684124-bc1df79b-57b2-49e0-af6f-dc362993992d.webp)

### `物理内存管理`

- 目前 xv6 里面简单的空闲链表法，引申到伙伴系统 Slab 分配器，不管什么地方，空间管理的方式一般就两种，链表和位图，万变不离其宗。

#### 位图管理

```cpp
struct bitmap
{                            // 这个数据结构就是用来管理整个位图
    uint32_t btmp_bytes_len; // 记录整个位图的大小，单位为字节
    uint8_t *bits;           // 用来记录位图的起始地址，我们未来用这个地址遍历位图时，操作单位指定为最小的字节
};
```

#### 内存池的规划

```cpp
/*管理虚拟内存池的数据结构*/
struct virtual_addr
{
    struct bitmap vaddr_map; // 管理虚拟内存池的位图
    uint32_t vaddr_start;    // 虚拟内存池的起始地址
};

/*管理物理内存池的数据结构*/
struct pool
{
    struct bitmap pool_bitmap; // 管理物理内存池的位图
    uint32_t phy_addr_start;   // 物理内存池的起始地址
    uint32_t pool_size;        // 物理内存池的大小
};
```

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740726779817-0490182c-6fef-4f4c-8831-03fb1fa283dd.png)

**物理内存池的规划**

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740726600551-28b291f1-18e5-435e-bff8-9ae486a426ac.png)

**虚拟内存池的规划**

**内核堆空间的起始虚拟地址：0xc0100000**

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740726693977-65ec0de4-2216-451c-9402-17036cd11aeb.png)



#### 位图存放

- **内核main线程的栈顶地址：0xc009f000**
- **内核main线程的PCB地址：0xc009e000**

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740726681737-82da2693-6b23-4ae8-b1c4-1bafb2ad6315.png)





#### 页内存分配

以下实现的是内核页内存的分配

**实现逻辑**

- 在**内核虚拟内存池**中寻找到**空闲的连续**page个虚拟页面
- 在**内核物理内存池**中找到**page个**物理页面（**可能不连续**）
- 逐一构**建找到的虚拟页面与物理页面之间的映射关系**（本质上是在**构建页表项和页目录项，然后装填页表和页目录表**）

虚拟内存池中寻找连续的空闲虚拟页

```cpp
/*
在pf所指向的虚拟内存池中寻找pg_cnt空闲虚拟页，并分配之
*/
static void *vaddr_get(enum pool_flags pf, uint32_t pg_cnt)
{
    int vaddr_start = 0, bit_idx_start = -1;
    if (pf == PF_KERNEL)
    {
        /*内核虚拟内存池*/
        // 寻找可用的连续页
        bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
        if (bit_idx_start == -1)
            return NULL;
        // 如果找到就将这些连续空间设置为已分配
        uint32_t cnt = 0;
        while (cnt < pg_cnt)
            bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1);
        vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
    }
    else
    {
        /*用户虚拟内存池*/
    }
    return (void *)vaddr_start;
}
```

物理内存池中寻找空闲的物理页

```cpp
/*
在m_pool(物理内存池数据结构)所指向的物理内存池中寻找一页空闲的物理页，并分配之
*/
static void *palloc(struct pool *m_pool)
{
    int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
    if (bit_idx == -1)
        return NULL;
    bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
    uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + m_pool->phy_addr_start);
    return (void *)page_phyaddr;
};
```

**不同于虚拟内存，物理内存很多时候是不连续的，而且我们也不需要他们是连续的，只需要虚拟内存是连续的，然后在页表中构建他们之间的映射关系即可**，因此在这里我们实现的是在物理内存池中寻找一页空闲的物理页

**构建申请的虚拟页和物理页之间映射关系**

```cpp
/*
页表中添加虚拟地址_vaddr和物理地址_page_phyaddr的映射关系
*/
static void page_table_add(void *_vaddr, void *_page_phyaddr)
{
    uint32_t vaddr = (uint32_t)_vaddr, page_phyaddr = (uint32_t)_page_phyaddr;
    uint32_t *pde = pde_ptr(vaddr);
    uint32_t *pte = pte_ptr(vaddr);
    /* 判断页目录内目录项的P位，若为1,则表示其指向的页表已存在，直接装填对应的页表项*/
    if (*pde & 0x00000001)
    {
        // 如果页目录项P位为1，说明页目录项指向的页表存在，但是页表项不存在，则直接创建页表项
        if (!(*pte & 0x00000001))
        {
            // 填充页目录项
            *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        }
        else
        {
            PANIC("pte repeat");
            *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        }
    }
    else
    {
        /*
        页目录项指向的页表不存在，需要首先构建页表，然后再装填对应的页目录项和页表项
        1.在物理内存池中寻找一页物理页分配给页表
        2.将该页表初始化为0，防止这块内存的脏数据乱入
        3.将该页表写入页目录项
        4.构建页表项，填充进页表
        */
        // 在内核物理内存池找一页物理页分配给页表
        uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);
        // 初始化页表清零
        /* 访问到pde对应的物理地址,用pte取高20位便可.
         * 因为pte是基于该pde对应的物理地址内再寻址,
         * 把低12位置0便是该pde对应的物理页的起始*/
        memset((void *)((int)pte & 0xfffff000), 0, PG_SIZE);
        // 将分配的页表物理地址写入页目录项
        *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        ASSERT(!(*pte & 0x00000001));
        // 构建页表项，填充进页表
        *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
    }
}
```

**分配pg_cnt个页空间，成功则返回起始虚拟地址**

```cpp
/*
分配pg_cnt个页空间，成功则返回起始虚拟地址，失败则返回NULL
*/
void *malloc_page(enum pool_flags pf, uint32_t pg_cnt)
{
    /***********   malloc_page的原理是三个动作的合成:   ***********
      1通过vaddr_get在虚拟内存池中申请虚拟地址
      2通过palloc在物理内存池中申请物理页
      3通过page_table_add将以上得到的虚拟地址和物理地址在页表中完成映射
    ***************************************************************/
    ASSERT(pg_cnt > 0 && pg_cnt < 3840);
    // 获取连续的pg_cnt个虚拟页
    void *vaddr_start = vaddr_get(pf, pg_cnt);
    if (vaddr_start == NULL)
        return NULL;
    uint32_t vaddr = (uint32_t)vaddr_start, cnt = pg_cnt;
    struct pool *mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;
    /*
    逐一申请物理页，然后再挨个映射进虚拟页
    之所以物理页不能一次性申请，是因为物理页可能是离散的
    但是虚拟页是连续的
    */
    while (cnt--)
    {
        void *page_phyaddr = palloc(mem_pool);
        if (page_phyaddr == NULL)
            return NULL;
        page_table_add((void *)vaddr, page_phyaddr);
        vaddr += PG_SIZE;
    }
    return vaddr_start;
}
```



### 堆、共享区等的管理方式

- *xv6 里面堆的管理方式也是链表，看侯捷老师关于 C++ 内存部分讲解，也是类似伙伴系统的思想，申请空间过大时也是 mmap 在共享区分配空间*

#### 空闲块的管理

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740835294391-cb9516ab-b59a-4cdb-9301-04fa9042d52f.png)

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740835232349-5818d98b-4415-4528-a69e-f3e5e01ff5b5.png)

空闲块的定义

```cpp
/*
内存块描述符个数
对应16、32、64、128、256、512、1024这7种小内存块
*/
#define DESC_CNT 7

/*内存块*/
struct mem_block
{
   // 内存块使用双链表进行管理
   struct list_elem free_elem;
};
```

总内存块的管理结构

```cpp
/*内存块描述符*/
struct mem_block_desc
{
   // 每个块的大小
   uint32_t block_size;
   // 块的总数量
   uint32_t blocks_per_arena;
   // 用于管理组织块的双链表结构的头节点
   struct list free_list;
};
```

空闲块的管理结构

```cpp
/*
内存仓库元信息
每个页面有7种类型大小的内存块
一个arena管理一种类型大小的内存块
*/
struct arena
{
	/*管理该种类型大小的内存块数组索引*/
	struct mem_block_desc *desc;
	/*
	标记是分配的页框还是内存块
	如果是页框，则cnt表示页框的数量
	如果是内存块，则cnt表示空闲内存块的数量
	（注意，是空闲块的数量，mem_block_desc中表示的是块的总数）
	*/
	bool large;
	uint32_t cnt;
};
```

#### 从哪申请内存块

##### 内核内存池

```cpp
/*不同类型大小的内存块管理数组*/
struct mem_block_desc k_block_descs[DESC_CNT];
```

##### 用户内存池

```cpp
/* 进程或线程的pcb,程序控制块, 此结构体用于存储线程的管理信息*/
struct task_struct
{
   uint32_t *self_kstack; // 用于存储线程的栈顶位置，栈顶放着线程要用到的运行信息
   pid_t pid;             // 定义线程或者进程的pid
   enum task_status status;
   uint8_t priority; // 线程优先级
   char name[16];    // 用于存储自己的线程的名字

   uint8_t ticks;                                // 线程允许上处理器运行还剩下的滴答值，因为priority不能改变，所以要在其之外另行定义一个值来倒计时
   uint32_t elapsed_ticks;                       // 此任务自上cpu运行后至今占用了多少cpu嘀嗒数, 也就是此任务执行了多久*/
   struct list_elem general_tag;                 // general_tag的作用是用于线程在一般的队列(如就绪队列或者等待队列)中的结点
   struct list_elem all_list_tag;                // all_list_tag的作用是用于线程队列thread_all_list（这个队列用于管理所有线程）中的结点
   uint32_t *pgdir;                              // 进程自己页目录表的虚拟地址
   struct virtual_addr userprog_vaddr;           // 每个用户进程自己的虚拟地址池
   struct mem_block_desc u_block_desc[DESC_CNT]; // 用户进程内存块描述符
   uint32_t stack_magic;                         // 如果线程的栈无限生长，总会覆盖地pcb的信息，那么需要定义个边界数来检测是否栈已经到了PCB的边界
};
```

上述主要为PCB数据结构添加内存块管理成员

```cpp
struct mem_block_desc u_block_desc[DESC_CNT]; // 用户进程内存块描述符
```

#### 内存分配算法

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740836630324-6453fffd-204d-4026-8524-3b6cc711cfd9.png)

1. 判断**在哪种类型的内存池中申请内存块**——内核内存池 or 用户内存池？
2. 判断**申请的内存大小是否大于最大可分配类型大小的内存1024B**

- - 如果申请内存的块大小，**大于最大可分配类型大小的内存1024B**，则说明无法分配小的内存块，**直接分配一页页框**给申请者
  - 否则转向步骤3

1. **遍历各种类型大小的内存块，找到第一个满足申请内存大小的类型内存（首次适配算法）**。如申请者申请500B的内存，遍历后发现256B<500B<512B，说明此时可以尝试去找一块空闲的512B的内存块分配给申请者
2. **查看空闲链表是否为空，即查看是否有没有被分配的512B内存块**

- - 如果**空闲链表为空**，说明512B大小的内存块还没有创建或者已经被分配完了

- - - 则此时重新**从内核中申请一页内存**
    - 然后**将该页内存划分成7个（由于有arena的存在，可用的块只有7个）512B大小的内存块**
    - 然后**将划分好的内存块插入到空闲链表**中

- - 如果空闲链表不为空，则转向5

1. 空闲链表不为空，说明**有可用的内存块使用，则从空闲链表中弹出然后分配给申请者**

#### 内存释放算法

内存块的释放是基于页面的——**假如所有内存块都空闲，则直接将该页面释放，否则就只是将该内存块插入到空闲链表中**

因此我们首先需要构建内存页的释放，内存页的释放是内存页分配的逆过程

1. 在物理内存池中释放物理内存页（只需将位图置为0即可）
2. 清除页表中的页表项，即清除掉虚拟内存和物理内存的映射关系
3. 在虚拟内存池中释放虚拟内存页（只需将位图置为0即可）

### `缺页异常三种，写时复制、延迟分配、页面置换`

- - `写时复制的实现思想`，要点：利用页表项保留位设置为COW标志位，复制内存时(主要fork)不实际复制内存，只复制页表，并将COW=1，R/W 设置为只读，其他共享计数、回收等略，详见 xv6 写时复制实现
  - *延迟分配*，页表项全0，只在页表中建立了映射，并未实际分配物理内存，详见 xv6 延迟分配实验
  - `页面置换`，页表项非空，页表项 P = 0;`常见页面置换算法`;`页面置换交换区实现方式`，具体实现有些复杂，详见 ucore 的例子，有配套的手册讲解。

### `Linux kmalloc 的特点`

- 分配的物理内存连续



## 进程管理

### 进程的设计

#### PCB

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740732033648-181ec709-413d-4e09-a182-6c0c1bb4398e.png)

- **位图是管理内存的数据结构**，对于线程或者进程，也需要有一个数据结构对其进行管理，这个数据结构就是PCB。
- **PCB（Process Control Block，进程控制块）是操作系统内部用于存储进程信息的数据结构**。

##### PCB 的生命周期

- 进程**创建**时：每当操作系统创建一个新的进程时，系统会为该进程分配一个PCB，初始化进程的各种信息；
- 进程**执行**时：进程在运行时，操作系统通过 PCB 来**管理和调度**进程。每当进程状态发生变化（如从就绪变为运行，或从运行变为阻塞），操作系统会更新 PCB；
- 进程**终止**时：当进程执行完毕或被终止时，操作系统会回收该进程的 PCB，并释放相关资源。

##### PCB的内容

PCB中包含了进程执行所需的各种信息，如进程状态、寄存器值、内存使用情况、I/O 状态等。

##### PCB 的主要功能

- **进程管理**：每个进程都有一个唯一的 PCB，操作系统通过它来追踪进程的状态、资源等信息。
- **上下文切换**：当操作系统切换执行进程时，它会保存当前进程的 PCB，并加载下一个进程的 PCB，从而实现进程的上下文切换。
- **进程调度**：操作系统通过PCB来选择下一个运行的进程。调度器根据进程的状态、优先级等信息做出决策。

### 内核线程的创建与运行

```cpp
/*定义线程栈，存储线程执行时的运行信息*/
struct thread_stack
{
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;

    // 一个函数指针，指向线程执行函数，目的是为了实现通用的线程函数调用
    void (*eip)(thread_func *func, void *func_args);
    // 以下三条是模仿call进入thread_start执行的栈内布局构建的，call进入就会压入参数与返回地址，因为我们是ret进入kernel_thread执行的
    // 要想让kernel_thread正常执行，就必须人为给它造返回地址，参数
    void(*unused_retaddr); // 一个栈结构占位
    thread_func *function;
    void *func_args;
};
```



```cpp
/*PCB结构体*/
struct task_struct
{
    // 线程栈的栈顶指针
    uint32_t *self_kstack;
    // 线程状态
    enum task_status status;
    // 线程的优先级
    uint8_t priority;
    // 线程函数名
    char name[16];
    // 用于PCB结构体的边界标记
    uint32_t stack_magic;
};
```

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740732165364-04338d4c-d223-4fce-a77d-14d42c7e6266.png)

**初始化PCB**

```cpp
/*初始化PCB*/
void init_thread(struct task_struct *pthread, char *name, int prio)
{
    memset(pthread, 0, sizeof(*pthread));
    strcpy(pthread->name, name);
    pthread->status = TASK_RUNNGING;
    pthread->priority = prio;
    /*
    一个线程的栈空间分配一页空间，将PCB放置在栈底
    pthread是申请的一页空间的起始地址，因此加上一页的大小，就是栈顶指针
    */
    pthread->self_kstack = (uint32_t *)((uint32_t)pthread + PG_SIZE);
    /*PCB的边界标记，防止栈顶指针覆盖掉PCB的内容*/
    pthread->stack_magic = 0x19991030;
}
```

**初始化线程栈的运行信息**

```cpp
/*根据PCB信息，初始化线程栈的运行信息*/
void thread_create(struct task_struct *pthread, thread_func function, void *func_args)
{
    /*给线程栈空间的顶部预留出中断栈信息的空间*/
    pthread->self_kstack = (uint32_t *)((int)(pthread->self_kstack) - sizeof(struct intr_stack));
    /*给线程栈空间的顶部预留出线程栈信息的空间*/
    pthread->self_kstack = (uint32_t *)((int)(pthread->self_kstack) - sizeof(struct thread_stack));
    // 初始化线程栈，保存线程运行时需要的信息
    struct thread_stack *kthread_stack = (struct thread_stack *)pthread->self_kstack;

    // 线程执行函数
    kthread_stack->eip = kernel_thread;
    kthread_stack->function = function;
    kthread_stack->func_args = func_args;
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->edi = kthread_stack->esi = 0;
}
```

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740733366032-c63eb8af-f7c0-4f22-a058-4b0b77b0d0b7.png)

**线程的执行**

```cpp
    /*4.上述准备好线程运行时的栈信息后，即可运行执行函数了*/
    asm volatile("movl %0,%%esp;    \
                pop %%ebp;          \
                pop %%ebx;          \
                pop %%edi;          \
                pop %%esi;          \
                ret"
                 :
                 : "g"(thread->self_kstack)
                 : "memory");
```



- **当线程栈初始化结束之后，栈顶指针首先弹出了寄存器映像**
- **这样栈顶指针就指向了通用执行函数kernel_thread，这样接下来只需要调用kernel_thread，就调用了用户的执行函数**

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740733468385-666f09f4-5b3c-47c5-a012-1792efbe88a1.png)

```cpp
static void kernel_thread(thread_func *function, void *func_args)
{
    function(func_args);
}
```

于是接下来代码执行ret指令,ret指令会做两件事

- **将当前栈顶指针的值(也就是kernel_thread的地址)弹出**
- 然后**赋值给指令寄存器EIP，这样就相当于调用了kernel_thread，**由于弹出了栈顶指针的值，因此栈顶指针会回退

于是接下来，根据c语言的函数调用约定

kernel_thread会取出占位的返回地址上边的两个参数，也就是执行函数的地址与执行函数的参数，然后调用执行函数运行

### 内核线程调度

#### 就绪队列与阻塞队列

为了实现多线程的轮转调度，我们需要使用队列将存储线程信息的PCB组织起来，如**就绪队列、阻塞队列**等，为此需要首先需要定义这种数据结构——**双向链表**

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740734410642-f6540eb5-e5f1-4a6c-a36d-09050a395fcf.png)

#### 基于时钟中断的轮转调度策略

##### 基本逻辑

- **每个线程在运行之前都被分配一个时间片**，当前线程**每执行一个时钟周期（每发生一次时钟中断），时间片就减一**，直到**减为0的时候就开始进行轮转调度**
- 时钟中断无时无刻都在发生，**每发生一次时钟中断都要触发一次时钟中断的中断处理程序**，因此我们只需要**修改时钟中断的中断处理程序，让其每次被调用的时候就将当前正在运行的线程可用时间（ticks）减去一，直到减为0的时候就调用调度函数**

##### 时钟中断处理程序

```cpp
uint32_t ticks; // ticks是内核自中断开启以来总共的嘀嗒数
/* 时钟的中断处理函数 */
static void intr_timer_handler(void)
{
   //获取当前正在执行的线程的pcb
   struct task_struct *cur_thread = running_thread();
   // 检查栈是否溢出
   ASSERT(cur_thread->stack_magic == 0x20241221);
   // 记录此线程占用的cpu时间嘀
   cur_thread->elapsed_ticks++;
   // 从内核第一次处理时间中断后开始至今的滴哒数,内核态和用户态总共的嘀哒数
   ticks++;

   if (cur_thread->ticks == 0)
   { // 若进程时间片用完就开始调度新的进程上cpu
      schedule();
   }
   else
   { // 将当前进程的时间片-1
      cur_thread->ticks--;
   }
}
```

然后把时钟中断处理程序注册到中断描述符表中，这样当时钟中断触发的时候就会执行该时钟中断处理程序

```cpp
void timer_init()
{
   put_str("timer_init start\n");
   /* 将时钟处理程序注册到IDT中 */
   register_handler(0x20, intr_timer_handler);
   put_str("timer_init done\n");
}
/* 在中断处理程序数组第vector_no个元素中注册安装中断处理程序function */
void register_handler(uint8_t vector_no, intr_handler function) {
/* idt_table数组中的函数是在进入中断后根据中断向量号调用的,
 * 见kernel/kernel.S的call [idt_table + %1*4] */
   idt_table[vector_no] = function; 
}
```



##### 调度策略

- 把当前**时间片已经用完的线程换到就绪队列尾**，并**修改其状态为就绪态**
- 然后从就绪队列头**取出一个新的线程换上执行新的时间片**，并修改其状态为**运行态**

```cpp
/*时间片轮转调度函数*/
void schedule(void)
{
    ASSERT(intr_get_status() == INTR_OFF);
    /*在关中断的情况下
    把当前时间片已经用完的线程换到就绪队列尾
    然后从就绪队列头取出一个新的线程换上执行新的时间片
    */
    struct task_struct *cur = running_thread();
    if (cur->status == TASK_RUNNING)
    {
        ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
        list_append(&thread_ready_list, &cur->general_tag);
        cur->ticks = cur->priority;
        cur->status = TASK_READY;
    }
    else
    {
        /* 若此线程需要某事件发生后才能继续上cpu运行,
          不需要将其加入队列,因为当前线程不在就绪队列中。*/
    }

    /* 将thread_ready_list队列中的第一个就绪线程弹出,准备将其调度上cpu. */
    ASSERT(!list_empty(&thread_ready_list));
    thread_tag = NULL; // thread_tag清空
    thread_tag = list_pop(&thread_ready_list);
    struct task_struct *next = elem2entry(struct task_struct, general_tag, thread_tag);
    next->status = TASK_RUNNING;
    switch_to(cur, next);
}
```

##### 调度之后上cpu运行，切换过程

###### 保护上下文环境

系统中的任务调度需保护任务两层执行流的上下文，分两部分完成：

1. **进入中断时的保护**：保存任务全部寄存器映像，即进入中断前任务所属层的状态，相当于任务中用户代码的上下文。由 `kermel.S` 中 `intr%lentry` 中断处理入口程序（由汇编宏 “% acrO VECTOR2” 定义，也被称为中断处理程序 “模板”）通过 `push` 指令完成。恢复这些寄存器后，若为用户进程则恢复在用户态执行，若为内核线程则恢复在内核态执行被中断的内核代码。
2. **保护内核环境上下文**：依据 ABI，除 `esp` 外，仅保护 `esi`、`edi`、`ebx` 和 `ebp` 这 4 个寄存器，其映像相当于任务中内核代码的上下文（第二层执行流），负责恢复在内核中断处理程序中继续执行的状态。后续需结合具体实现解释原因。

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740749092674-c5ef2a68-56b5-4ef7-bc9b-9e255b8989d1.png)

###### 切换栈顶指针

- 将**当前正在运行的栈顶指针(esp)**和寄存器映像保存到**旧PCB**中
- 将**新PCB中的栈顶指针取出赋值给esp**准备执行
- 借用**ret指令弹出新线程的可执行函数地址，赋值给指令寄存器eip**，正式运行新线程的执行函数

```cpp
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
```



### 用户进程

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740819203710-f480d92a-5587-4c3c-bb9f-fef17e9e9f76.png)

#### 用户进程的创建

![img](https://raw.githubusercontent.com/jl-sky/imageDatesets/master/2025/01/upgit_20250105_1736066965.png)![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740798784033-d1d57446-02e3-49a6-815d-b60b26aa9987.png)



用户进程的PCB将复用内核线程的PCB内容，为了让每个进程都拥有自己的地址空间，为pcb新增两个数据成员

- **页目标表pgdir**

- - **每个进程的页目录表的768号项到1022号项（指向内核空间的页目录项）都会从内核main线程中复制过来，表示每个进程都会共享内核空间**
  - 当pthread->pgdir = NULL时，表示这是个线程，因为线程没有自己的地址空间，页就没有自己的页目录表

- 初始化进程自己的**虚拟内存池**

```cpp
   uint32_t *pgdir;                    // 进程自己页目录表的虚拟地址
   struct virtual_addr userprog_vaddr; // 每个用户进程自己的虚拟地址池
```

##### 分配进程独立的页表（初始化pcb的时候做）

```cpp
uint32_t *create_page_dir(void)
{
   // 用户进程的页表不能让用户直接访问到,所以在内核空间来申请
   uint32_t *page_dir_vaddr = get_kernel_pages(1);
   if (page_dir_vaddr == NULL)
   {
      console_put_str("create_page_dir: get_kernel_page failed!");
      return NULL;
   }
   // 将内核页目录表的768号项到1022号项复制过来
   memcpy((uint32_t *)((uint32_t)page_dir_vaddr + 768 * 4), (uint32_t *)(0xfffff000 + 768 * 4), 255 * 4);
   uint32_t new_page_dir_phy_addr = addr_v2p((uint32_t)page_dir_vaddr);       // 将进程的页目录表的虚拟地址，转换成物理地址
   page_dir_vaddr[1023] = new_page_dir_phy_addr | PG_US_U | PG_RW_W | PG_P_1; // 页目录表最后一项填自己的地址，为的是动态操作页表
   return page_dir_vaddr;
}
```

##### **分配独立的地址空间（虚拟内存池）**

```cpp
// 初始化进程pcb中的用于管理自己虚拟地址空间的虚拟内存池结构体
void create_user_vaddr_bitmap(struct task_struct *user_prog)
{
   // 虚拟内存池的起始地址
   user_prog->userprog_vaddr.vaddr_start = USER_VADDR_START;
   /*
   (0xc0000000 - USER_VADDR_START)表示用户栈的大小
   故，(0xc0000000 - USER_VADDR_START) / PG_SIZE表示用户栈占用的页面数
   由于位图中每个比特位对应记录的是一个页面
   因此(0xc0000000 - USER_VADDR_START) / PG_SIZE / 8表示用户栈位图所占用的比特数
   DIV_ROUND_UP表示向上取整，如9/10=1
   因此整行代码表示管理用户栈的位图所需要的内存大小（页面数）
   */
   uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PG_SIZE / 8, PG_SIZE);
   // 从内核中申请为位图存放的空间
   user_prog->userprog_vaddr.vaddr_bitmap.bits = get_kernel_pages(bitmap_pg_cnt);
   // 计算出位图长度（字节单位）
   user_prog->userprog_vaddr.vaddr_bitmap.btmp_bytes_len = (0xc0000000 - USER_VADDR_START) / PG_SIZE / 8;
   // 初始化位图
   bitmap_init(&user_prog->userprog_vaddr.vaddr_bitmap);
}
```

##### 初始化进程运行需要的栈空间（初始化中断栈的时候做）

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740799871977-338e88af-da86-43be-9aad-3f2834770583.png)

- 在4GB的虚拟空间中，**(0xc0000000-1)是用户空间的最高地址，0xc0000000~0xffffffff是内核空间**。
- 效仿这种内存结构布局，把用户空间的最高处即0xc0000000-1，及以下的部分内存空间用于存储用户进程的命令行参数，之下的空间再作为用户的栈和堆。所以用户栈的栈底地址为0xc0000000。
- 由于在申请内存时，内存管理模块返回的地址是内存空间的下边界，所以我们为栈申请的地址应该是(0xc0000000-0x1000)，此地址是用户栈空间栈顶的下边界。这里我们用宏来定义此地址，即USER STACK3VADDR它定义在 userprogh中。

```cpp
proc_stack->esp = (void *)((uint32_t)get_a_page(PF_USER, USER_STACK3_VADDR) + PG_SIZE);
```



##### 将准备好的pcb插入就绪队列

```cpp
   ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
   list_append(&thread_ready_list, &thread->general_tag);

   ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
   list_append(&thread_all_list, &thread->all_list_tag);
   intr_set_status(old_status);
```

#### 用户进程的运行

**进程的运行是由时钟中断调用schedule，由调用器 schedule 调度实现的**。

- **schedule会首先激活要切换的进程的页表**
- **然后调用switch to，准备切换进程执行**
- **switch to最终执行kernel_thread**
- **kernel_thread执行start_process**
- **start_process负责准备中断返回的环境（主要是准备要切换的特权级），同时开辟栈帧空间**
- **最终iret指令中断返回，切换用户态特权级，同时弹出真正要执行的用户函数，然后去执行**

#### 页表切换

- **每个进程都拥有独立的虚拟地址空间，本质上就是各个进程都有单独的页表**
- **页表是存储在页表寄存器 CR3 中的，CR3寄存器只有1个**，因此，不同的**进程在执行前，我们要在CR3寄存器中为其换上与之配套的页表，从而实现了虚拟地址空间的隔离**

```cpp
void page_dir_activate(struct task_struct *p_thread)
{
   /********************************************************
    * 执行此函数时,当前任务可能是线程。
    * 之所以对线程也要重新安装页表, 原因是上一次被调度的可能是进程,
    * 否则不恢复页表的话,线程就会使用进程的页表了。
    ********************************************************/

   /* 若为内核线程,需要重新填充页表为0x100000 */
   uint32_t pagedir_phy_addr = 0x100000; // 默认为内核的页目录物理地址,也就是内核线程所用的页目录表
   if (p_thread->pgdir != NULL)
   { // 如果不为空，说明要调度的是个进程，那么就要执行加载页表，所以先得到进程页目录表的物理地址
      pagedir_phy_addr = addr_v2p((uint32_t)p_thread->pgdir);
   }
   asm volatile("movl %0, %%cr3" : : "r"(pagedir_phy_addr) : "memory"); // 更新页目录寄存器cr3,使新页表生效
}
```



##### 特权级切换

从高特权级到低特权级的切换只有通过中断返回才能进行，当中断返回时，处理器

- 用栈中的数据作为返回地址加载栈中 eflags的值到 efags 寄存器
- 如果栈中 cs.rpl若为更低的特权级，处理器的特权级检查通过后，会将栈中 cs载入到 CS寄存器，栈中ss载入 SS寄存器，随后处理器进入低特权级。
- 需要注意的是由于cs和ss中存放的要切换的特权级，因此当弹出cs和ss就是准备好切换特权级了

设定即将要切换的特权级（即用户进程的特权级，当是在内核态下）

```cpp
   proc_stack->cs = SELECTOR_U_CODE;
   // 设置用户态下的eflages的相关字段
   proc_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
   proc_stack->ss = SELECTOR_U_DATA;
```

最后使用中断返回指令切换到用户态的特权级

```cpp
asm volatile("movl %0, %%esp; jmp intr_exit" : : "g"(proc_stack) : "memory");
```



- `谈进程、线程、协程的理解，聊区别`，*简易设计*
- `为什么切换进程比切换线程慢`，除了进程“体量”大，另外切换进程要切换页表，切换页表要刷新TLB
- `切换时机`，自己阻塞让出 CPU，时间片到了
- `状态转换，发生中断时进程状态如何转换`，时间中断设置为就绪，其他情况在 xv6 里面没变，Linux 不了解。

#### `关于进程的命令`

#### 如何查看进程状态等等

## 文件管理

[何为文件索引节点?文件与磁盘的爱恨情仇。_哔哩哔哩_bilibili](https://www.bilibili.com/video/BV1jy4y1K73r/?spm_id_from=333.337.search-card.all.click&vd_source=23fecc1a0310f387371490d0c2975c12)

- `写磁盘如何保证数据一致性，设计日志层`（当时某度二面，全程基本就讨论这个）
- `inode、路径、目录、目录项、全局文件表和文件结构体、进程打开文件表和文件描述符等概念以及它们之间的关系`
- `open close dup write read 等常见系统调用`，做了什么事情

- - 比如 open 的要点分配文件结构体、文件描述符。
  - 这个函数只是用户接口，整个是一个是系统调用过程。

- `inode 相当于树形结构的索引、假如设计为哈希索引，如何设计，Cache 缓存，优劣等等讨论`
- `目录项缓存`，Linux 使用目录项缓存 dentry cache 缓存提高目录项对象的处理效率，我也只知道这个东西，待研究
- `命令获取某个目录下的文件数量`

### FCB

#### inode

文件系统为实现文件管理方案，必然会创造出一些辅助管理的数据结构，**只要用于管理、控制文件相关信息的数据结构都被称为FCB(File ContrlBlock)，即文件控制块**,inode 也是这种结构，因此 inode是 FCB 的一种，同时目录项也是FCB的一种。

创建文件的本质就是创建inode和目录项

- 前12号项直接存储文件块的地址
- 第13号项存储一级索引块的地址（一级索引块可存储256个块地址）
- 第14号项存储二级索引块的地址
- 第15号项存储三级索引块的地址

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740880169028-34aa673a-5ab6-421a-b0b1-9379d1a5a753.png)

inode 的数量等于文件的数量，为方便管理，分区中所有文件的 inode 通过一个大表格来维护，此表格称为inode table，在计算机中表格都可以用数组来表示，因此inode table 本质上就是 inode 数组，数组元素的下标便是文件 inode 的编号。

#### 目录与目录项

- **目录也是文件，需要用inode管理**，因此目录也称为目录文件
- **操作系统只认识inode**，因此**区分目录文件和普通的文件的区别就是他们的内容**
- **普通文件**的内容是**数据**，而**目录文件**的内容是**目录项**



![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740881190627-4fde810a-6397-461b-a10f-9f7368d940ae.png)

有了目录项后，通过文件名找文件实体数据块的流程是

(1)在目录中找到文件名所在的目录项。

(2)从目录项中获取 imode 编号。

(3)用inode 编号作为 inode 数组的索引下标，找到 inode。

(4)从该inode中获取数据块的地址，读取数据块。

#### inode与目录项的关系

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740881474601-4aa985d9-9b34-4e73-be68-fc2022e6330f.png)

```
硬链接、软链接区别
```

- - **硬链接与目录项挂钩，多个目录项一个 inode 一个文件**
  - **软链接是个文件，有自己的 inode，文件内容是路径**

#### 超级块

- `分区布局`，引导块->超级块->inode、位图、数据区

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740881574966-6777e4a7-34a1-4047-8819-f441c9ea397c.png)![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740881626575-9a874436-6f9d-415e-abc8-18bd1bd59bb8.png)

### 基本数据结构

#### 目录项结构

```cpp
/*目录项结构*/
struct dir_entry
{
    // 普通文件或目录名称
    char filename[MAX_FILE_NAME_LEN];
    // 普通文件或目录对应的inode编号
    uint32_t i_no;
    // 文件类型
    enum file_types f_type;
};
```

#### 目录结构

struct dir 是**目录结构，并不在磁盘上存在**，**只用于与目录相关的操作时，在内存中创建的结构**，用过之后就释放了，不会回写到磁盘中

```cpp
/*目录结构*/
struct dir
{
    struct inode *inode;
    // 记录在目录内的偏移，即目录项的偏移量
    uint32_t dir_pos;
    // 目录的数据缓存
    uint8_t dir_buf[512];
};
```

#### inode结构

```cpp
/*inode结构*/
struct inode
{
    // inode编号
    uint32_t i_no;

    /* 当此inode是文件时,i_size是指文件大小,
    若此inode是目录,i_size是指该目录下所有目录项大小之和*/
    uint32_t i_size;
    // 记录此文件被打开的次数
    uint32_t i_open_cnts;
    // 写文件不能并行,进程写文件前检查此标识，类似于锁机制
    bool write_deny;

    /*
    i_sectors[0-11]是直接块,
    i_sectors[12]用来存储一级间接块指针
    */
    uint32_t i_sectors[13];
    // 将文件内容读取进内存之后使用双链表进行组织
    struct list_elem inode_tag;
};
```

#### 超级块

```cpp
/*超级块*/
struct super_block
{
    // 标识文件系统
    uint32_t magic;
    // 本分区的起始lba地址
    uint32_t part_lba_base;
    // 本分区的总共扇区数
    uint32_t sec_cnt;

    // 分区中inode数量
    uint32_t inode_cnt;

    /*块位图，用于管理数据块*/
    // 块位图的lba地址
    uint32_t block_bitmap_lba;
    // 块位图占据的扇区数
    uint32_t block_bitmap_sects;

    /*inode位图，用于管理inode，一个inode对应一个文件*/
    // inode位图的lba地址
    uint32_t inode_bitmap_lba;
    // inode位图占据的扇区数
    uint32_t inode_bitmap_sects;

    // inode数组的lba地址
    uint32_t inode_table_lba;
    // inode数组占据的扇区数
    uint32_t inode_table_sects;

    // 数据区的第一个扇区号
    uint32_t data_start_lba;
    // 根目录的inode号
    uint32_t root_inode_no;
    // 目录项的大小
    uint32_t dir_entry_size;

    // 填充字段，让超级块占据完整的一个块（扇区）的大小
    uint8_t pad[460];
} __attribute__((packed));
```

### 文件描述符

- 每个进程的PCB中都有一个文件描述符数组
- 数组中的元素是打开文件表的索引，这就是文件描述符
- 打开文件表的每个表项都记录了打开的某个文件的操作情况（例如文件指针的位置等），以及inode

```cpp
/* 文件结构 */
struct file
{
    uint32_t fd_pos; // 记录当前文件操作的偏移地址,以0为起始,最大为文件大小-1
    uint32_t fd_flag;
    struct inode *fd_inode;
};
/* 文件表 */
struct file file_table[MAX_FILE_OPEN];
```

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740897365102-842a61ab-06c2-4edd-a82b-fc3eb0602035.png)

每个进程都有三个标准的文件描述符，0是标准输入，1是标准输出，2是标准错误

## 同步机制与互斥锁

### 线程的阻塞与解除阻塞过程

阻塞

- **调用者负责**将当前线程**插入阻塞队列**
- 修改当前运行**线程的状态为阻塞态**
- 从**就绪队列头部****中取出新的pcb上处理器运行**

```cpp
void thread_block(enum task_status status)
{
    ASSERT(((status == TASK_BLOCKED) || (status == TASK_HANGING) || (status == TASK_WAITING)));
    enum intr_status old_status = intr_get_status();
    /*修改当前线程状态为阻塞态
    然后调用调度器从就绪队列摘取一块新线程执行*/
    struct task_struct *cur_pthread = running_thread();
    cur_pthread->status = status;
    schedule();

    intr_set_status(old_status);
}
```

解除阻塞

- **调用者负责**将当前线程**从阻塞队列中移除**
- **修改**运行**状态为就绪态**
- 将当前pcb**插入到****就绪队列头部**

```cpp
void thread_unblock(struct task_struct *pthread)
{
    enum intr_status old_status = intr_get_status();
    /*修改PCB状态为就绪态，同时插入就绪队列头部，优先调用*/
    enum task_status status = pthread->status;
    ASSERT(((status == TASK_BLOCKED) || (status == TASK_HANGING) || (status == TASK_WAITING)));
    if (status != TASK_READY)
    {
        if (elem_find(&thread_ready_list, &pthread->general_tag))
            PANIC("thread_unblock: blocked thread in ready_list\n");
        list_append(&thread_ready_list, &pthread->general_tag);
        pthread->status = TASK_READY;
    }

    intr_set_status(old_status);
}
```



### 信号量的实现

```cpp
// 用于初始化信号量，传入参数就是指向信号量的指针与初值
void sema_init(struct semaphore *psema, uint8_t value)
{
    psema->value = value;       // 为信号量赋初值
    list_init(&psema->waiters); // 初始化信号量的等待队列
}

/*信号量的P操作*/
void sema_down(struct semaphore *psema)
{
    // 对阻塞队列公共资源的访问需要关中断以避免访问过程中被打断
    enum intr_status old_status = intr_disable();

    // 如果当前可用资源（信号量的值）为0，则应当阻塞当前线程
    while (psema->value == 0)
    {
        if (elem_find(&psema->waiters, &running_thread()->general_tag))
            PANIC("sema_down: thread blocked has been in waiters_list\n");
        list_append(&psema->waiters, &running_thread()->general_tag);
        thread_block(TASK_BLOCKED);
    }
    // 可以让当前线程访问公共资源，同时让可访问的资源数减去一
    psema->value--;
    ASSERT(psema->value == 0);
    // 恢复中断之前的状态
    intr_set_status(old_status);
}

// 信号量的up操作，也就是+1操作，传入参数是指向要操作的信号量的指针。且释放信号量时，应唤醒阻塞在该信号量阻塞队列上的一个进程
void sema_up(struct semaphore *psema)
{
    /* 关中断,保证原子操作 */
    enum intr_status old_status = intr_disable();
    ASSERT(psema->value == 0);
    if (!list_empty(&psema->waiters))
    { // 判断信号量阻塞队列应为非空，这样才能执行唤醒操作
        struct task_struct *thread_blocked = elem2entry(struct task_struct, general_tag, list_pop(&psema->waiters));
        thread_unblock(thread_blocked);
    }
    psema->value++;
    ASSERT(psema->value == 1);
    /* 恢复之前的中断状态 */
    intr_set_status(old_status);
}
```



### `按下一个键到显示在屏幕上`

#### 处理逻辑

- 键盘中断 + 显卡、写显存 过程，太多略，详见[捋一捋控制台的输入输出](http://mp.weixin.qq.com/s?__biz=Mzg3MDU3ODcxMQ==&mid=2247485490&idx=1&sn=4fb3a691101eedae28eff0b6f2ecea34&chksm=ce8ae74af9fd6e5cef631bbd81bf55d1b3519a3a6d3a0c788ea4f7870cae145953b9a8da2d7e&scene=21#wechat_redirect)

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740793757960-08c19d50-30c9-4def-ab2b-c1fcd61fdb07.png)

- 当按键被按下后，键的状态改变，键盘中的 8048 芯片把按键对应的扫描码(通码或断码)发送到主板上的 8042 芯片
- 8042处理后保存在自己的寄存器中，然后向 8259A 发送中断信号
- 处理器执行键盘中断处理程序，将 8042处理过的扫描码从寄存器中读取出来，继续进行下一步处理。

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740793816950-5b15b61b-ad03-488f-a7c8-55048516eb80.png)

#### 键盘中断处理程序

- 当键盘键入按键后，8048芯片就将扫描码发送给8042，然后8042触发中断信号，接着触发中断处理程序
- 键盘中断对应的中断处理程序的逻辑就是读取8042接收到的扫描码，然后将其按照扫描码与键盘的对应关系显示在键盘上

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740794472253-37adefca-58f8-42d8-8037-794e19a18996.png)

![img](https://cdn.nlark.com/yuque/0/2025/png/53299329/1740794856049-786dd8d1-f6d7-43a0-89df-fd457d375dd4.png)



- `锁的实现`，自旋锁和休眠锁
- `自旋锁`，while 循环
- `内存一致性`，硬件如何支持的原子操作，聊了指令 Lock 锁总线等等，CAS、Acquire/Released 等
- `休眠锁涉及进程的休眠唤醒机制如何实现`
- `Cache 缓存一致性`，MESI
- `设计操作系统需要研究研究设计模式，所以设计模式`？？？？？？
- `复杂指令集和精简指令集区别`
- `elf 文件格式介绍，可装载段，装载地址，filesz、memsz 等`
