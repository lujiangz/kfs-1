#include "keyboard.h"
#include "kernel.h"

/* US Keyboard Layout */
unsigned char kbdus[128] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',		/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,		/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
'\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

void keyboard_handler() {
    uint8_t scancode = inb(0x60);

    /* En üst bit 1 ise tuş bırakıldı demektir (Release). */
    if (scancode & 0x80) {
        // Tuş bırakıldı.
    } else {
        // Tuş basıldı.
        // DEBUG: Hangi tuşa basıldığını görmek için scancode'u yazdır.
        // printk("[%x]", scancode); 
        
        if (scancode == 0x3B) { // F1
             terminal_switch_screen(0);
        } else if (scancode == 0x3C) { // F2
             terminal_switch_screen(1);
        } else if (scancode == 0x3D) { // F3
             terminal_switch_screen(2);
        } else if (scancode < 128) {
            char c = kbdus[scancode];
            if (c) {
                printk("%c", c);
            }
        }
    }

    /* PIC'e EOI (End of Interrupt) gönder */
    outb(0x20, 0x20);
}