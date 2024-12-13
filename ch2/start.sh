nasm mbr.S -o mbr.bin
dd if=$(pwd)/mbr.bin  of=~/bochs/hd60M.img conv=notrunc
