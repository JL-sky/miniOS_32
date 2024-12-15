nasm -f elf32 syscall_write.S -o syscall_write.o
ld -m elf_i386 syscall_write.o -o syscall_write.bin

