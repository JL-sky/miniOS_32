# 定义路径
PWD := $(shell pwd)
BOOT_DIR := $(PWD)/boot
LIB_DIR := $(PWD)/lib
BIN_DIR := $(PWD)/bin
HD_IMG := /home/minios/bochs/hd60M.img 

# 文件
MBR_SRC := $(BOOT_DIR)/mbr.S
LOADER_SRC := $(BOOT_DIR)/loader.S
PRINT_SRC := $(LIB_DIR)/kernel/print.S
MAIN_SRC := $(PWD)/kernel/main.c

# 编译器和工具
NASM := nasm
GCC := gcc-4.4
LD := ld
DD := dd

# 标志
NASM_FLAGS := -I $(BOOT_DIR)/include/
GCC_FLAGS := -m32 -I $(LIB_DIR)/kernel
LD_FLAGS := -m elf_i386 -Ttext 0xc0001500 -e main
DD_FLAGS := bs=512 conv=notrunc

# 文件目标
MBR_BIN := $(BIN_DIR)/mbr
LOADER_BIN := $(BIN_DIR)/loader
PRINT_OBJ := $(BIN_DIR)/print.o
MAIN_OBJ := $(BIN_DIR)/main.o
KERNEL_BIN := $(BIN_DIR)/kernel.bin

# 默认目标
all: $(HD_IMG)

$(HD_IMG): $(MBR_BIN) $(LOADER_BIN) $(KERNEL_BIN)
	# 写入 MBR
	$(DD) if=$(MBR_BIN) of=$(HD_IMG) count=1 $(DD_FLAGS)
	# 写入 LOADER
	$(DD) if=$(LOADER_BIN) of=$(HD_IMG) count=4 seek=2 $(DD_FLAGS)
	# 写入内核文件
	$(DD) if=$(KERNEL_BIN) of=$(HD_IMG) count=200 seek=9 $(DD_FLAGS)

# 编译 MBR
$(MBR_BIN): $(MBR_SRC)
	$(NASM) -o $@ $(NASM_FLAGS) $<

# 编译 LOADER
$(LOADER_BIN): $(LOADER_SRC)
	$(NASM) -o $@ $(NASM_FLAGS) $<

# 编译 print.o
$(PRINT_OBJ): $(PRINT_SRC)
	$(NASM) -f elf32 -o $@ $<

# 编译 main.o
$(MAIN_OBJ): $(MAIN_SRC)
	$(GCC) -o $@ -c $(GCC_FLAGS) $<

# 链接内核文件
$(KERNEL_BIN): $(MAIN_OBJ) $(PRINT_OBJ)
	$(LD) $(LD_FLAGS) -o $@ $^

# 清理
clean:
	rm -f $(BIN_DIR)/* 

