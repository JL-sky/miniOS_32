nasm -I ./include/ ./mbr.S -o mbr.bin
nasm -I ./include/ ./loader.S -o loader.bin
dd if=/home/minios/osCode/miniOS_32/ch4/mbr.bin of=/home/minios/bochs/hd60M.img bs=512 count=1 conv=notrunc
dd if=/home/minios/osCode/miniOS_32/ch4/loader.bin of=/home/minios/bochs/hd60M.img bs=512 count=2 seek=2 conv=notrunc
