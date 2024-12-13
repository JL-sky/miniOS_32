# 定义路径变量
BIN_DIR = $(shell pwd)/bin
BOOT_DIR = $(shell pwd)/boot
BOOT_INCLUDE_DIR = $(BOOT_DIR)/include/
MBR_SRC = mbr.S
LOADER_SRC = loader.S
MBR_BIN = mbr.bin
LOADER_BIN = loader.bin

KERNEL_DIR = $(shell pwd)/lib/kernel
KERNEL_SRC = main.c
KERNEL_OBJ = main.o
KERNEL_BIN = kernel.bin

# 目标文件
IMG = /home/minios/bochs/hd60M.img

# 工具
NASM = nasm
GCC = gcc-4.4
LD = ld
DD = dd

# 默认目标
all: $(IMG)

# 创建所需的目录
$(BIN_DIR):
	mkdir -p $@

# 生成 mbr.bin
$(BIN_DIR)/$(MBR_BIN): $(BOOT_DIR)/$(MBR_SRC) $(BOOT_INCLUDE_DIR)
	$(NASM) -I $(BOOT_INCLUDE_DIR) $(BOOT_DIR)/$(MBR_SRC) -o $(BIN_DIR)/$(MBR_BIN)

# 生成 loader.bin
$(BIN_DIR)/$(LOADER_BIN): $(BOOT_DIR)/$(LOADER_SRC) $(BOOT_INCLUDE_DIR)
	$(NASM) -I $(BOOT_INCLUDE_DIR) $(BOOT_DIR)/$(LOADER_SRC) -o $(BIN_DIR)/$(LOADER_BIN)

# 生成 kernel.o
$(BIN_DIR)/$(KERNEL_OBJ): $(KERNEL_DIR)/$(KERNEL_SRC)
	$(GCC) $(KERNEL_DIR)/$(KERNEL_SRC) -c -m32 -o $(BIN_DIR)/$(KERNEL_OBJ)

# 链接内核
$(BIN_DIR)/$(KERNEL_BIN): $(BIN_DIR)/$(KERNEL_OBJ)
	$(LD) -m elf_i386 -Ttext 0x00001500 -e main -o $(BIN_DIR)/$(KERNEL_BIN) $(BIN_DIR)/$(KERNEL_OBJ)

# 写入 mbr.bin 到镜像
$(IMG): $(BIN_DIR)/$(MBR_BIN) $(BIN_DIR)/$(LOADER_BIN) $(BIN_DIR)/$(KERNEL_BIN)
	$(DD) if=$(BIN_DIR)/$(MBR_BIN) of=$(IMG) bs=512 count=1 conv=notrunc
	$(DD) if=$(BIN_DIR)/$(LOADER_BIN) of=$(IMG) bs=512 count=4 seek=2 conv=notrunc
	$(DD) if=$(BIN_DIR)/$(KERNEL_BIN) of=$(IMG) bs=512 count=200 seek=9 conv=notrunc

# 清理
clean:
	rm -rf $(BIN_DIR)

# 伪目标
.PHONY: all clean

