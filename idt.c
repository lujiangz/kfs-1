#include "idt.h"

idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;

extern void idt_load(uint32_t);
extern void isr33(); // Assembly wrapper for keyboard

void set_idt_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_low = base & 0xFFFF;
    idt_entries[num].base_high = (base >> 16) & 0xFFFF;

    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    idt_entries[num].flags   = flags; 
}

/* PIC Remapping */
#define PIC1		0x20		/* IO base for master PIC */
#define PIC2		0xA0		/* IO base for slave PIC */
#define PIC1_COMMAND	PIC1
#define PIC1_DATA	(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	(PIC2+1)

void pic_remap() {
    uint8_t a1, a2;

    a1 = inb(PIC1_DATA);                        // save masks
    a2 = inb(PIC2_DATA);

    outb(PIC1_COMMAND, 0x11);  // starts the initialization sequence (in cascade mode)
    outb(PIC2_COMMAND, 0x11);

    outb(PIC1_DATA, 0x20);                 // ICW2: Master PIC vector offset (32)
    outb(PIC2_DATA, 0x28);                 // ICW2: Slave PIC vector offset (40)

    outb(PIC1_DATA, 0x04);                 // ICW3: tell Master PIC that there is a slave PIC at IRQ2
    outb(PIC2_DATA, 0x02);                 // ICW3: tell Slave PIC its cascade identity

    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);
    
    outb(PIC1_DATA, 0xFD); 
    // 0xFC = 1111 1100.
    outb(PIC2_DATA, 0xFF);
}

void init_idt() {
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    memset(&idt_entries, 0, sizeof(idt_entry_t) * 256);

    pic_remap();

    // IRQ 1 is Keyboard. Remapped to 32+1 = 33.
    set_idt_gate(33, (uint32_t)isr33, 0x08, 0x8E); // 0x08 is kernel code segment

    idt_load((uint32_t)&idt_ptr);
}
