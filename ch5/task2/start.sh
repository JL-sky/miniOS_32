nasm -I ./include/ ./mbr.S -o mbr.bin
nasm -I ./include/ ./loader.S -o loader.bin
dd if=$(pwd)/mbr.bin of=/home/minios/bochs/hd60M.img bs=512 count=1 conv=notrunc
dd if=$(pwd)/loader.bin of=/home/minios/bochs/hd60M.img bs=512 count=4 seek=2 conv=notrunc
