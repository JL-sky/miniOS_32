# 源文件和目标文件
MBR_SRC = ./mbr.S
LOADER_SRC = ./loader.S
MBR_BIN = mbr.bin
LOADER_BIN = loader.bin
IMG = /home/minios/bochs/hd60M.img
SRC_DIR = $(shell pwd)

# 编译器和工具
NASM = nasm
DD = dd

# 默认目标
all: $(MBR_BIN) $(LOADER_BIN) $(IMG)

# 生成 mbr.bin
$(MBR_BIN): $(MBR_SRC)
	$(NASM) -I ./include/ $(MBR_SRC) -o $(MBR_BIN)

# 生成 loader.bin
$(LOADER_BIN): $(LOADER_SRC)
	$(NASM) -I ./include/ $(LOADER_SRC) -o $(LOADER_BIN)

# 将 mbr.bin 和 loader.bin 写入镜像
$(IMG): $(MBR_BIN) $(LOADER_BIN)
	$(DD) if=$(SRC_DIR)/$(MBR_BIN) of=$(IMG) bs=512 count=1 conv=notrunc
	$(DD) if=$(SRC_DIR)/$(LOADER_BIN) of=$(IMG) bs=512 count=2 seek=2 conv=notrunc

# 清理目标文件
clean:
	rm -f $(MBR_BIN) $(LOADER_BIN)

# 伪目标，避免与文件名冲突
.PHONY: all clean

