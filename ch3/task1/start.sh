nasm mbr.S -o mbr.bin
dd if=$(pwd)/mbr.bin of=/home/minios/bochs/hd60M.img bs=512 count=1 conv=notrunc
