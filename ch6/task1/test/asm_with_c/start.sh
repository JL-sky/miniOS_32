nasm -f elf32 ./C_with_S_S.S -o C_with_S_S.o
gcc-4.4 -m32 -c ./C_with_S_c.c -o C_with_S_c.o
ld -m elf_i386 ./C_with_S_c.o C_with_S_S.o -o main
