AS = as --32
CC = gcc
LD = ld
GRUB = grub-mkrescue --fonts="" --locales="" --themes="" --install-modules="multiboot normal terminal boot"

CFLAGS = -m32 -ffreestanding -fno-builtin -fno-exceptions -fno-stack-protector -nostdlib -nodefaultlibs
LDFLAGS = -m elf_i386 -T linker.ld 

KERNEL_BIN = kernel.bin 
ISO_NAME = myos.iso

OBJS = boot.o kernel_utils.o kernel.o gdt.o idt.o keyboard.o

all: $(ISO_NAME)

$(KERNEL_BIN) : $(OBJS)
		$(LD) $(LDFLAGS) -o $@ $^

boot.o: boot.s
		$(AS) -o $@ $<

kernel.o: kernel.c
		$(CC) $(CFLAGS) -c $< -o $@

kernel_utils.o: kernel_utils.c
	$(CC) $(CFLAGS) -c $< -o $@

gdt.o: gdt.c
	$(CC) $(CFLAGS) -c $< -o $@

idt.o: idt.c
	$(CC) $(CFLAGS) -c $< -o $@

keyboard.o: keyboard.c
	$(CC) $(CFLAGS) -c $< -o $@

$(ISO_NAME) : $(KERNEL_BIN)
		mkdir -p isodir/boot/grub
		cp $(KERNEL_BIN) isodir/boot/$(KERNEL_BIN)
		echo 'menuentry "KFS-1" { multiboot /boot/kernel.bin }' > isodir/boot/grub/grub.cfg
		$(GRUB) -o $(ISO_NAME) isodir

clean:
	rm -rf isodir
	rm -f *.o $(KERNEL_BIN) $(ISO_NAME)

run: $(ISO_NAME)
	qemu-system-i386 -cdrom $(ISO_NAME)

