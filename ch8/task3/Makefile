#定义一大堆变量，实质就是将需要多次重复用到的语句定义一个变量方便使用与替换
PWD=$(shell pwd)
BIN_DIR=$(PWD)/bin
ENTRY_POINT=0xc0001500
HD60M_PATH=/home/minios/bochs/hd60M.img 
#只需要把hd60m.img路径改成自己环境的路径，整个代码直接make all就完全写入了，能够运行成功
AS=nasm
CC=gcc-4.4
LD=ld
LIB= -I lib/ -I lib/kernel/ -I lib/user/ -I kernel/ 
ASFLAGS= -f elf
CFLAGS= -Wall $(LIB) -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes -m32
#-Wall warning all的意思，产生尽可能多警告信息，-fno-builtin不要采用内部函数，
#-W 会显示警告，但是只显示编译器认为会出现错误的警告
#-Wstrict-prototypes 要求函数声明必须有参数类型，否则发出警告。-Wmissing-prototypes 必须要有函数声明，否则发出警告

LDFLAGS= -Ttext $(ENTRY_POINT) -e main -Map $(BIN_DIR)/kernel.map -m elf_i386
#-Map,生成map文件，就是通过编译器编译之后，生成的程序、数据及IO空间信息的一种映射文件
#里面包含函数大小，入口地址等一些重要信息

OBJS=$(BIN_DIR)/main.o $(BIN_DIR)/init.o \
	$(BIN_DIR)/interrupt.o  $(BIN_DIR)/kernel.o \
	$(BIN_DIR)/print.o $(BIN_DIR)/debug.o

# 编译mbr和loader
boot:$(BIN_DIR)/mbr.o $(BIN_DIR)/loader.o
$(BIN_DIR)/mbr.o:boot/mbr.S
	$(AS) -I boot/include/ -o bin/mbr.o boot/mbr.S
	
$(BIN_DIR)/loader.o:boot/loader.S
	$(AS) -I boot/include/ -o bin/loader.o boot/loader.S
	
# 编译c内核代码
$(BIN_DIR)/main.o:kernel/main.c
	$(CC) $(CFLAGS) -o $@ $<	
# $@表示规则中目标文件名的集合这里就是$(BIN_DIR)/main.o  $<表示规则中依赖文件的第一个，这里就是kernle/main.c 

$(BIN_DIR)/init.o:kernel/init.c
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_DIR)/interrupt.o:kernel/interrupt.c
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_DIR)/debug.o:kernel/debug.c
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_DIR)/string.o:lib/debug.c
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_DIR)/bitmap.o:lib/kernel/bitmap.c
	$(CC) $(CFLAGS) -o $@ $<
# 编译汇编内核代码
$(BIN_DIR)/kernel.o:kernel/kernel.S 
	$(AS) $(ASFLAGS) -o $@ $<

$(BIN_DIR)/print.o:lib/kernel/print.S
	$(AS) $(ASFLAGS) -o $@ $<

# 链接所有内核代码
$(BIN_DIR)/kernel.bin:$(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^
# $^表示规则中所有依赖文件的集合，如果有重复，会自动去重

.PHONY:mk_dir hd clean build all boot	#定义了6个伪目标
mk_dir:
	if [ ! -d $(BIN_DIR) ];then mkdir $(BIN_DIR);fi 
#判断bin文件夹是否存在，如果不存在，则创建

hd:
	dd if=bin/mbr.o of=$(HD60M_PATH) count=1 bs=512 conv=notrunc && \
	dd if=bin/loader.o of=$(HD60M_PATH) count=4 bs=512 seek=2 conv=notrunc && \
	dd if=$(BIN_DIR)/kernel.bin of=$(HD60M_PATH) bs=512 count=200 seek=9 conv=notrunc
	
clean:
	@cd $(BIN_DIR) && rm -f ./* && echo "remove ./bin all done"
#-f, --force忽略不存在的文件，从不给出提示，执行make clean就会删除build下所有文件

build:$(BIN_DIR)/kernel.bin
#执行build需要依赖kernel.bin，但是一开始没有，就会递归执行之前写好的语句编译kernel.bin

all:mk_dir boot build hd
#make all 就是依次执行mk_dir build hd