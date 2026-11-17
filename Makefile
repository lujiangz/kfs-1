ARCH := $(shell uname -m)

ifneq (,$(filter aarch64 arm64,$(ARCH)))
    # ARM host (örn. Kali ARM / UTM) → cross-compiler kullan
    CROSS = i686-linux-gnu-
    AS = $(CROSS)as --32
    CC = $(CROSS)gcc
    LD = $(CROSS)ld
    GRUB = grub-mkrescue --directory=/usr/lib/grub/i386-pc --fonts="" --locales="" --themes="" --install-modules="multiboot normal terminal boot"
else
    # x86 / x86_64 host → native araçlar
    AS = as --32
    CC = gcc
    LD = ld
    GRUB = grub-mkrescue --fonts="" --locales="" --themes="" --install-modules="multiboot normal terminal boot"
endif


CFLAGS = -m32 -ffreestanding -fno-builtin -fno-exceptions -fno-stack-protector -fno-omit-frame-pointer -nostdlib -nodefaultlibs
LDFLAGS = -m elf_i386 -T linker.ld

KERNEL_BIN = kernel.bin
ISO_NAME = myos.iso

OBJS = boot.o kernel_utils.o kernel.o gdt.o idt.o keyboard.o

ifneq (,$(filter aarch64 arm64,$(ARCH)))
    # ARM'da varsayılan: sadece kernel (ISO için grub-pc-bin gerekiyor)
    all: $(KERNEL_BIN)
	@echo "✓ $(KERNEL_BIN) oluşturuldu (ARM cross-compile)"
	@echo "  Çalıştır: make run"
else
    all: $(ISO_NAME)
endif

kernel: $(KERNEL_BIN)
	@echo "✓ $(KERNEL_BIN) oluşturuldu ($(ARCH) host)"

$(KERNEL_BIN): $(OBJS)
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

iso: $(KERNEL_BIN)
	mkdir -p isodir/boot/grub
	cp $(KERNEL_BIN) isodir/boot/$(KERNEL_BIN)
	echo 'menuentry "KFS-1" { multiboot /boot/kernel.bin }' > isodir/boot/grub/grub.cfg
	$(GRUB) -o $(ISO_NAME) isodir
	@echo "✓ $(ISO_NAME) oluşturuldu"

$(ISO_NAME): $(KERNEL_BIN)
	mkdir -p isodir/boot/grub
	cp $(KERNEL_BIN) isodir/boot/$(KERNEL_BIN)
	echo 'menuentry "KFS-1" { multiboot /boot/kernel.bin }' > isodir/boot/grub/grub.cfg
	$(GRUB) -o $(ISO_NAME) isodir

clean:
	rm -rf isodir
	rm -f *.o

fclean: clean
	rm -f $(KERNEL_BIN) $(ISO_NAME)

re: fclean all

# ARM: QEMU multiboot kerneli doğrudan boot edebilir (ISO gerekmez)
# x86: ISO ile boot
ifneq (,$(filter aarch64 arm64,$(ARCH)))
run: $(KERNEL_BIN)
	qemu-system-i386 -kernel $(KERNEL_BIN)
else
run: $(ISO_NAME)
	qemu-system-i386 -cdrom $(ISO_NAME)
endif

# ── Bilgi ────────────────────────────────────────────────
info:
	@echo "Host Arch : $(ARCH)"
	@echo "CC        : $(CC)"
	@echo "AS        : $(AS)"
	@echo "LD        : $(LD)"

.PHONY: all kernel iso clean fclean re run info
