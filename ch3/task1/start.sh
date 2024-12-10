nasm mbr.S -o mbr.bin
dd if=/home/minios/osCode/miniOS_32/ch3/task1/mbr.bin of=/home/minios/bochs/hd60M.img bs=512 count=1 conv=notrunc
