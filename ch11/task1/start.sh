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
# 编译switch
nasm -f elf32 -o $(pwd)/bin/switch.o $(pwd)/thread/switch.S

#编译main文件
gcc-4.4 -o $(pwd)/bin/main.o -c -fno-builtin -m32 -I $(pwd)/lib/kernel/ -I $(pwd)/lib/ -I $(pwd)/kernel/ -I $(pwd)/device/ -I $(pwd)/thread/ -I $(pwd)/userprog/ $(pwd)/kernel/main.c
#编译interrupt文件
gcc-4.4 -o $(pwd)/bin/interrupt.o -c -fno-builtin -m32 -I $(pwd)/lib/kernel/ -I $(pwd)/lib/ -I $(pwd)/kernel/ $(pwd)/kernel/interrupt.c
#编译init文件
gcc-4.4 -o $(pwd)/bin/init.o -c -fno-builtin -m32 -I $(pwd)/lib/kernel/ -I $(pwd)/lib/ -I $(pwd)/kernel/ -I $(pwd)/thread/ -I $(pwd)/device/ -I $(pwd)/userprog/ $(pwd)/kernel/init.c
# 编译debug文件
gcc-4.4 -o $(pwd)/bin/debug.o -c -fno-builtin -m32 -I $(pwd)/lib/kernel/ -I $(pwd)/lib/ -I $(pwd)/kernel/ $(pwd)/kernel/debug.c
# 编译string文件
gcc-4.4 -o $(pwd)/bin/string.o -c -fno-builtin -m32 -I $(pwd)/lib/kernel/ -I $(pwd)/lib/ -I $(pwd)/kernel/ $(pwd)/lib/string.c
# 编译bitmap文件
gcc-4.4 -o $(pwd)/bin/bitmap.o -c -fno-builtin -m32 -I $(pwd)/lib/kernel/ -I $(pwd)/lib/ -I $(pwd)/kernel/ $(pwd)/lib/kernel/bitmap.c
# 编译memory文件
gcc-4.4 -o $(pwd)/bin/memory.o -c -fno-builtin -m32 -I $(pwd)/lib/kernel/ -I $(pwd)/lib/ -I $(pwd)/kernel/ $(pwd)/kernel/memory.c
# 编译thread文件
gcc-4.4 -o $(pwd)/bin/thread.o -c -fno-builtin -m32 -I $(pwd)/lib/kernel/ -I $(pwd)/lib/ -I $(pwd)/kernel/ -I $(pwd)/thread/ $(pwd)/thread/thread.c
# 编译list文件
gcc-4.4 -o $(pwd)/bin/list.o -c -fno-builtin -m32 -I $(pwd)/lib/kernel/ -I $(pwd)/lib/ -I $(pwd)/kernel/ $(pwd)/lib/kernel/list.c
# 编译timer文件
gcc-4.4 -o $(pwd)/bin/timer.o -c -fno-builtin -m32 -I $(pwd)/lib/kernel/ -I $(pwd)/lib/ -I $(pwd)/kernel/ -I $(pwd)/thread/ $(pwd)/device/timer.c
# 编译sync文件
gcc-4.4 -o $(pwd)/bin/sync.o -c -fno-builtin -m32 -I $(pwd)/lib/kernel/ -I $(pwd)/lib/ -I $(pwd)/kernel/ -I $(pwd)/thread/ $(pwd)/thread/sync.c
# 编译console文件
gcc-4.4 -o $(pwd)/bin/console.o -c -fno-builtin -m32 -I $(pwd)/lib/kernel/ -I $(pwd)/lib/ -I $(pwd)/kernel/ -I $(pwd)/thread/ $(pwd)/device/console.c
# 编译keyboard文件
gcc-4.4 -o $(pwd)/bin/keyboard.o -c -fno-builtin -m32 -I $(pwd)/lib/kernel/ -I $(pwd)/lib/ -I $(pwd)/kernel/ -I $(pwd)/thread/ $(pwd)/device/keyboard.c
# 编译ioqueue文件
gcc-4.4 -o $(pwd)/bin/ioqueue.o -c -fno-builtin -m32 -I $(pwd)/lib/kernel/ -I $(pwd)/lib/ -I $(pwd)/kernel/ -I $(pwd)/thread/ $(pwd)/device/ioqueue.c
# 编译tss文件
gcc-4.4 -o $(pwd)/bin/tss.o -c -fno-builtin -m32 -I $(pwd)/lib/kernel/ -I $(pwd)/lib/ -I $(pwd)/kernel/ -I $(pwd)/thread/ -I $(pwd)/userprog/ $(pwd)/userprog/tss.c


#将main函数与print函数进行链接
ld -m elf_i386 -Ttext 0xc0001500 -e main -o $(pwd)/bin/kernel.bin $(pwd)/bin/main.o $(pwd)/bin/kernel.o  $(pwd)/bin/init.o $(pwd)/bin/tss.o $(pwd)/bin/thread.o $(pwd)/bin/switch.o $(pwd)/bin/list.o $(pwd)/bin/sync.o $(pwd)/bin/console.o $(pwd)/bin/keyboard.o $(pwd)/bin/timer.o $(pwd)/bin/ioqueue.o $(pwd)/bin/interrupt.o $(pwd)/bin/memory.o $(pwd)/bin/bitmap.o  $(pwd)/bin/print.o  $(pwd)/bin/string.o $(pwd)/bin/debug.o

#将内核文件写入磁盘，loader程序会将其加载到内存运行
dd if=$(pwd)/bin/kernel.bin of=~/bochs/hd60M.img bs=512 count=200 conv=notrunc seek=9

#rm -rf bin/*