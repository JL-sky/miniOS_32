# 工具和文件路径定义
NASM = nasm
GCC = gcc-4.4
LD = ld
DD = dd
MKDIR = mkdir -p
PWD := $(shell pwd)
BIN_DIR = $(PWD)/bin
IMG = /home/minios/bochs/hd60M.img 
INCLUDE_DIR = $(PWD)/boot/include/
KERNEL_SRC_DIR = $(PWD)/kernel
LIB_DIR = $(PWD)/lib/kernel
BOOT_DIR = $(PWD)/boot

# 编译目标
MBR_SRC = $(BOOT_DIR)/mbr.S
LOADER_SRC = $(BOOT_DIR)/loader.S
PRINT_SRC = $(LIB_DIR)/print.S
KERNEL_SRC = $(KERNEL_SRC_DIR)/kernel.S
MAIN_SRC = $(KERNEL_SRC_DIR)/main.c
INTERRUPT_SRC = $(KERNEL_SRC_DIR)/interrupt.c
INIT_SRC = $(KERNEL_SRC_DIR)/init.c

MBR_BIN = $(BIN_DIR)/mbr
LOADER_BIN = $(BIN_DIR)/loader
PRINT_OBJ = $(BIN_DIR)/print.o
KERNEL_OBJ = $(BIN_DIR)/kernel.o
MAIN_OBJ = $(BIN_DIR)/main.o
INTERRUPT_OBJ = $(BIN_DIR)/interrupt.o
INIT_OBJ = $(BIN_DIR)/init.o
KERNEL_BIN = $(BIN_DIR)/kernel.bin

# 默认目标
all: $(BIN_DIR) $(MBR_BIN) $(LOADER_BIN) $(PRINT_OBJ) $(KERNEL_OBJ) $(MAIN_OBJ) $(INTERRUPT_OBJ) $(INIT_OBJ) $(KERNEL_BIN)

# 创建bin目录
$(BIN_DIR):
	$(MKDIR) $(BIN_DIR)

# 编译mbr
$(MBR_BIN): $(MBR_SRC)
	$(NASM) -o $@ -I $(INCLUDE_DIR) $<

# 编译loader
$(LOADER_BIN): $(LOADER_SRC)
	$(NASM) -o $@ -I $(INCLUDE_DIR) $<

# 编译print函数
$(PRINT_OBJ): $(PRINT_SRC)
	$(NASM) -f elf32 -o $@ $<

# 编译kernel
$(KERNEL_OBJ): $(KERNEL_SRC)
	$(NASM) -f elf32 -o $@ $<

# 编译main函数
$(MAIN_OBJ): $(MAIN_SRC)
	$(GCC) -o $@ -c -fno-builtin -m32 -I $(LIB_DIR) -I $(KERNEL_SRC_DIR) -I $(LIB_DIR) $<

# 编译interrupt函数
$(INTERRUPT_OBJ): $(INTERRUPT_SRC)
	$(GCC) -o $@ -c -fno-builtin -m32 -I $(LIB_DIR) -I $(KERNEL_SRC_DIR) -I $(LIB_DIR) $<

# 编译init函数
$(INIT_OBJ): $(INIT_SRC)
	$(GCC) -o $@ -c -fno-builtin -m32 -I $(LIB_DIR) -I $(KERNEL_SRC_DIR) -I $(LIB_DIR) $<

# 链接生成kernel.bin
$(KERNEL_BIN): $(MAIN_OBJ) $(PRINT_OBJ) $(KERNEL_OBJ) $(INTERRUPT_OBJ) $(INIT_OBJ)
	$(LD) -m elf_i386 -Ttext 0xc0001500 -e main -o $@ $^

# 将内核文件写入磁盘映像
install: $(KERNEL_BIN)
	$(DD) if=$(KERNEL_BIN) of=$(IMG) bs=512 count=200 conv=notrunc seek=9

# 将mbr写入磁盘映像
install-mbr: $(MBR_BIN)
	$(DD) if=$(MBR_BIN) of=$(IMG) bs=512 count=1 conv=notrunc

# 将loader写入磁盘映像
install-loader: $(LOADER_BIN)
	$(DD) if=$(LOADER_BIN) of=$(IMG) bs=512 count=4 seek=2 conv=notrunc

# 执行安装（mbr、loader、kernel）
install-all: install-mbr install-loader install

# 清理生成的文件
clean:
	rm -rf $(BIN_DIR)/*

.PHONY: all clean install install-mbr install-loader install-all
