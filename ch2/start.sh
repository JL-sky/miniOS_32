nasm mbr.S -o mbr.bin
dd if=~/osCode/miniOS_32/ch2/mbr.bin  of=~/bochs/hd60M.img conv=notrunc
