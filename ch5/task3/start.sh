#!/bin/bash

# 设置工作目录和工具
BIN_DIR=$(pwd)/bin
BOOT_DIR=$(pwd)/boot
BOOT_INCLUDE_DIR=$BOOT_DIR/include/
KERNEL_DIR=$(pwd)/lib/kernel
IMG=/home/minios/bochs/hd60M.img

NASM=nasm
GCC=gcc-4.4
LD=ld
DD=dd

# 目标文件
MBR_SRC=$BOOT_DIR/mbr.S
LOADER_SRC=$BOOT_DIR/loader.S
KERNEL_SRC=$KERNEL_DIR/main.c

MBR_BIN=$BIN_DIR/mbr.bin
LOADER_BIN=$BIN_DIR/loader.bin
KERNEL_OBJ=$BIN_DIR/main.o
KERNEL_BIN=$BIN_DIR/kernel.bin

# 创建目录
echo "创建目标目录..."
mkdir -p $BIN_DIR
mkdir -p $BOOT_DIR

# 编译 MBR
echo "编译 MBR..."
$NASM -I $BOOT_INCLUDE_DIR $MBR_SRC -o $MBR_BIN

# 编译 Loader
echo "编译 Loader..."
$NASM -I $BOOT_INCLUDE_DIR $LOADER_SRC -o $LOADER_BIN

# 编译内核
echo "编译内核..."
$GCC $KERNEL_SRC -c -m32 -o $KERNEL_OBJ

# 链接内核
echo "链接内核..."
$LD -m elf_i386 -Ttext 0x00001500 -e main -o $KERNEL_BIN $KERNEL_OBJ

# 创建启动镜像
echo "创建启动镜像..."
$DD if=$MBR_BIN of=$IMG bs=512 count=1 conv=notrunc
$DD if=$LOADER_BIN of=$IMG bs=512 count=4 seek=2 conv=notrunc
$DD if=$KERNEL_BIN of=$IMG bs=512 count=200 seek=9 conv=notrunc

# 完成
echo "构建完成！镜像文件已创建：$IMG"

# 退出脚本
exit 0

