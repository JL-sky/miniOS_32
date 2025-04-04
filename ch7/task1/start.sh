mkdir -p bin
#编译mbr
nasm -o $(pwd)/bin/mbr -I $(pwd)/boot/include/ $(pwd)/boot/mbr.S
dd if=$(pwd)/bin/mbr of=~/bochs/hd60M.img bs=512 count=1 conv=notrunc

#编译loader
nasm -o $(pwd)/bin/loader -I $(pwd)/boot/include/ $(pwd)/boot/loader.S
dd if=$(pwd)/bin/loader of=~/bochs/hd60M.img bs=512 count=4 seek=2 conv=notrunc
 
#编译print函数
nasm -f elf32 -o $(pwd)/bin/print.o $(pwd)/lib/kernel/print.S
# 编译kernel
nasm -f elf32 -o $(pwd)/bin/kernel.o $(pwd)/kernel/kernel.S

#编译main函数
gcc-4.4 -o $(pwd)/bin/main.o -c -fno-builtin -m32 -I $(pwd)/lib/kernel/ -I $(pwd)/lib/ -I $(pwd)/kernel/ $(pwd)/kernel/main.c
#编译interrupt
gcc-4.4 -o $(pwd)/bin/interrupt.o -c -fno-builtin -m32 -I $(pwd)/lib/kernel/ -I $(pwd)/lib/ -I $(pwd)/kernel/ $(pwd)/kernel/interrupt.c
#编译init
gcc-4.4 -o $(pwd)/bin/init.o -c -fno-builtin -m32 -I $(pwd)/lib/kernel/ -I $(pwd)/lib/ -I $(pwd)/kernel/ $(pwd)/kernel/init.c

#将main函数与print函数进行链接
ld -m elf_i386 -Ttext 0xc0001500 -e main -o $(pwd)/bin/kernel.bin $(pwd)/bin/main.o $(pwd)/bin/print.o $(pwd)/bin/init.o $(pwd)/bin/interrupt.o $(pwd)/bin/kernel.o

#将内核文件写入磁盘，loader程序会将其加载到内存运行
dd if=$(pwd)/bin/kernel.bin of=~/bochs/hd60M.img bs=512 count=200 conv=notrunc seek=9

#rm -rf bin/*
