# 定义目标文件和源文件
MBR_SRC = ./mbr.S
LOADER_SRC = ./loader.S
MBR_BIN = mbr.bin
LOADER_BIN = loader.bin
IMG = /home/minios/bochs/hd60M.img

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

# 将 mbr.bin 写入镜像
$(IMG): $(MBR_BIN) $(LOADER_BIN)
	$(DD) if=$(MBR_BIN) of=$(IMG) bs=512 count=1 conv=notrunc
	$(DD) if=$(LOADER_BIN) of=$(IMG) bs=512 count=2 seek=2 conv=notrunc

# 清理生成的文件
clean:
	rm -rf $(MBR_BIN) $(LOADER_BIN)
