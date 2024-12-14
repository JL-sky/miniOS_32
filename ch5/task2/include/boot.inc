;-----loader and kernel-----
    LOADER_BASE_ADDR equ 0x900                      ;loader在内存中位置
    LOADER_START_SECTOR equ 0x2                     ;loader在磁盘中的逻辑扇区地址，即LBA地址

    LOADER_STACK_TOP equ LOADER_BASE_ADDR           ;初始化栈指针地址

    PAGE_DIR_TABLE_POS equ 0x100000                 ;页目录表基址

;-----  gdt描述符属性  ---------
    DESC_G_4K   equ   1_00000000000000000000000b    ;设置段界限的单位为4KB
    DESC_D_32   equ    1_0000000000000000000000b    ;设置代码段/数据段的有效地址（段内偏移地址）及操作数大小为32位
    DESC_L    equ       0_000000000000000000000b    ;64位代码段标记位，现在是32位操作系统，因此标记为0即可。
    DESC_AVL  equ        0_00000000000000000000b

;定义段界限位
    ;段界限的第2部分，即描述符的高32位中的第16~19位，最终的代码段段界限为0xFFFFF
    DESC_LIMIT_CODE2 equ 1111_0000000000000000b     ;定义代码段要用的段描述符高32位中16~19段界限为全1
    DESC_LIMIT_DATA2 equ DESC_LIMIT_CODE2           ;定义数据段要用的段描述符高32位中16~19段界限为全1
    DESC_LIMIT_VIDEO2  equ 0000_000000000000000b    ;定义我们要操作显存时对应的段描述符的高32位中16~19段界限为全0
    DESC_P  equ 1_000000000000000b                  ;定义了段描述符中的P标志位，表示该段描述符指向的段是否在内存中

;定义描述符的特权级别位
    DESC_DPL_0 equ 00_0000000000000b
    DESC_DPL_1 equ 01_0000000000000b
    DESC_DPL_2 equ 10_0000000000000b
    DESC_DPL_3 equ 11_0000000000000b
;定义类型位
    DESC_S_CODE equ 1_000000000000b                 ;代码段和数据段都是非系统段，故类型字段s设置为1
    DESC_S_DATA equ DESC_S_CODE                     ;代码段和数据段都是非系统段，故类型字段s设置为1
    DESC_S_sys equ 0_000000000000b                  ;系统段的类型字段设置为0
;定义子类型位
    DESC_TYPE_CODE equ 1000_00000000b               ;x=1,c=0,r=0,a=0 代码段是可执行的，非一致性，不可读，已访问位a清0
    DESC_TYPE_DATA equ 0010_00000000b               ;x=0,e=0,w=1,a=0，数据段不可执行，向上扩展，可写，已访问位a清0

;拼接代码段的描述符
DESC_CODE_HIGH4 equ (0x00<<24) + DESC_G_4K \
                    + DESC_D_32 + DESC_L + \
                    DESC_AVL + DESC_LIMIT_CODE2 + DESC_P + DESC_DPL_0 + \
                    DESC_S_CODE + DESC_TYPE_CODE + 0x00
;拼接数据段的描述符
DESC_DATA_HIGH4 equ (0x00<<24) + DESC_G_4K \
                    + DESC_D_32 + DESC_L + \
                    DESC_AVL + DESC_LIMIT_DATA2 + DESC_P + DESC_DPL_0 + \
                    DESC_S_DATA + DESC_TYPE_DATA + 0x00

;拼接显存段的描述符位                    
DESC_VIDEO_HIGH4 equ (0x00<<24) + DESC_G_4K \
                    + DESC_D_32 + DESC_L + \
                    DESC_AVL + DESC_LIMIT_VIDEO2 + DESC_P + DESC_DPL_0 + \
                    DESC_S_DATA + DESC_TYPE_DATA + 0x0b

;-----  选择子属性  ---------
    RPL0 equ 00b
    RPL1 equ 01b
    RPL2 equ 10b
    RPL3 equ 11b

    TI_GDT equ 000b
    TI_LDT equ 100b


;-----  模块化的页目录表字段  ---------
    PG_P equ 1b
    PG_RW_R equ 00b
    PG_RW_W equ 10b
    PG_US_S equ 000b
    PG_US_U equ 100b
