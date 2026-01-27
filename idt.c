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

    // Unmask only Keyboard (IRQ 1). Bit 1 of IMR.
    // Actually we should restore masks but ensure Keyboard is unmasked.
    // For now, let's just unmask keyboard and cascade (IRQ 2).
    // Master: 1111 1101 (0xFD) -> enable Keyboard (bit 1) and Cascade (bit 2)?
    // IRQ 1 is bit 1 (2nd bit). IRQ 0 is bit 0.
    // Mask: 0 = enabled, 1 = disabled.
    // ~2 = 1111 1101 (FD).
    // ~1 = 1111 1110 (FE).
    
    // Let's restore masks for now, assuming they were 0xFF (all disabled) or we should explicitly enable keyboard.
    // If I restore `a1`, I don't know what it was.
    // Let's explicitly enable keyboard.
    
    outb(PIC1_DATA, 0xFD); // Enable Keyboard (IRQ1) and everything else disabled? No, IRQ2 is cascade.
    // Actually, usually we enable IRQ0 (Timer) and IRQ1 (Keyboard).
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
