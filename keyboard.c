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

    /* Key release — ignore */
    if (scancode & 0x80) {
        goto eoi;
    }

    /* F1–F5: Screen switching */
    if (scancode == 0x3B) { /* F1 */
        terminal_switch_screen(0);
        /* Restore input boundary for this screen */
        goto eoi;
    }
    if (scancode == 0x3C) { /* F2 */
        terminal_switch_screen(1);
        goto eoi;
    }
    if (scancode == 0x3D) { /* F3 */
        terminal_switch_screen(2);
        goto eoi;
    }
    if (scancode == 0x3E) { /* F4 — Tester */
        terminal_switch_screen(3);
        run_tests();
        /* After tests, switch to view result */
        terminal_switch_screen(3);
        goto eoi;
    }
    if (scancode == 0x3F) { /* F5 — Log */
        terminal_switch_screen(4);
        goto eoi;
    }
    
    /* Only process keyboard input on user screens (0, 1, 2) */
    if (current_screen >= 3)
        goto eoi;

    /* Translate scancode */
    if (scancode < 128) {
        char c = kbdus[scancode];
        if (!c) goto eoi;

        if (c == '\n') {
            /* Enter — process command */
            printk("\n");
            cmd_buffer[cmd_len] = '\0';
            process_command(cmd_buffer);
            print_prompt();
        } else if (c == '\b') {
            /* Backspace — remove from buffer and screen */
            if (cmd_len > 0) {
                cmd_len--;
                cmd_buffer[cmd_len] = '\0';
                terminal_putchar('\b');
            }
        } else {
            /* Printable character */
            if (cmd_len < CMD_BUFFER_SIZE - 1) {
                cmd_buffer[cmd_len++] = c;
                cmd_buffer[cmd_len] = '\0';
                printk("%c", c);
            }
        }
    }

eoi:
    /* Send EOI to PIC */
    outb(0x20, 0x20);
}