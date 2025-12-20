AS = i686-elf-as
CC = i686-elf-gcc
LD = i686-elf-ld
GRUB = grub-mkrescue --fonts="" --locales="" --themes="" --install-modules="multiboot normal terminal boot"

CFLAGS = -ffreestanding -fno-builtin -fno-exceptions -fno-stack-protector -nostdlib -nodefaultlibs -Wall -Wextra
LDFLAGS = -T linker.ld 

KERNEL_BIN = kernel.bin 
ISO_NAME = myos.iso

OBJS = boot.o kernel_utils.o kernel.o

all: $(KERNEL_BIN)

$(KERNEL_BIN): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

boot.o: boot.s
	$(AS) -o $@ $<

kernel.o: kernel.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel_utils.o: kernel_utils.c
	$(CC) $(CFLAGS) -c $< -o $@

$(ISO_NAME): $(KERNEL_BIN)
	mkdir -p isodir/boot/grub
	cp $(KERNEL_BIN) isodir/boot/$(KERNEL_BIN)
	echo 'menuentry "KFS-1" { multiboot /boot/kernel.bin }' > isodir/boot/grub/grub.cfg
	$(GRUB) -o $(ISO_NAME) isodir

clean:
	rm -rf isodir
	rm -f *.o $(KERNEL_BIN) $(ISO_NAME)

run: $(KERNEL_BIN)
	qemu-system-i386 -kernel $(KERNEL_BIN)

run-iso: $(ISO_NAME)
	qemu-system-i386 -cdrom $(ISO_NAME)

.PHONY: all clean run run-iso build-kernel