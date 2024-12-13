nasm -I ./include/ ./mbr.S -o mbr.bin
nasm -I ./include/ ./loader.S -o loader.bin
dd if=mbr.bin of=/home/minios/bochs/hd60M.img bs=512 count=1 conv=notrunc
dd if=loader.bin of=/home/minios/bochs/hd60M.img bs=512 count=2 seek=2 conv=notrunc
