/* 
Multiboot header için sabitler
*/

.set ALIGN, 1<<0
.set MEMINFO, 1<<1
.set FLAGS, ALIGN | MEMINFO
.set MAGIC, 0x1BADB002      /* Bootloader bu magicnumber'i görünce kernel olduğunu anlıyor. */
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.align 4     /* Verileri 32-bit (4 byte) sınırına hizala. İşlemci ve Bootloader'ın doğru okuması için şarttır. */
.long MAGIC
.long FLAGS
.long CHECKSUM

/* 
Stack oluşturmamız lazım
*/

.section .bss
.align 16   /* Kullanacağımız derleyiciler System V ABI standartlarına uyduğu için 16 ve katı olan bir değer ile başlatmamız gerekiyor. (SSE (Streaming SIMD Extensions))*/
.global stack_bottom
.global stack_top
stack_bottom:
.skip 16384  /* 16kib */
stack_top:

.section .text
.global _start
.type _start, @function
_start:
    mov $stack_top, %esp
    xor %ebp, %ebp
	cld
    call kernel_main
    cli
1:  hlt
    jmp 1b

.size _start, . - _start

.global gdt_flush
.type gdt_flush, @function
gdt_flush:
    mov 4(%esp), %eax
    lgdt (%eax)
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    ljmp $0x08, $.flush
.flush:
    ret

.global idt_load
.type idt_load, @function
idt_load:
    mov 4(%esp), %eax
    lidt (%eax)
    ret

.global isr33
.type isr33, @function
isr33:
    pusha
	cld
	mov %esp, %ebx
	and $-16, %esp
    call keyboard_handler
	mov %ebx, %esp
    popa
    iret
