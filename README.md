# kfs-1

https://wiki.osdev.org/Bare_Bones

sudo apt install build-essential nasm qemu-system-x86 xorriso grub-pc-bin gcc-multilib

as --32 boot.s -o boot.o

gcc -m32 -ffreestanding -fno-builtin -fno-exceptions -c kernel.c -o kernel.o

ld -m elf_i386 -T linker.ld -o kernel.bin boot.o kernel.o

grub-mkrescue -o myos.iso isodir

qemu-system-i386 -cdrom myos.iso