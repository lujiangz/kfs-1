#include "kernel.h"

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer = (uint16_t*) VGA_MEMORY;

/* Ekranı bir satır yukarı kaydırır */
void terminal_scroll(void) {
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            terminal_buffer[y * VGA_WIDTH + x] = terminal_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
    }
}

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
    update_cursor(0, 0);
}

void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        terminal_row++;
    } else {
        terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
        if (++terminal_column == VGA_WIDTH) {
            terminal_column = 0;
            terminal_row++;
        }
    }

    if (terminal_row == VGA_HEIGHT) {
        terminal_scroll();
        terminal_row = VGA_HEIGHT - 1;
    }
    update_cursor(terminal_column, terminal_row);
}

void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++)
        terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) {
    terminal_write(data, strlen(data));
}

void kernel_main(void) {
    terminal_initialize();

    printk("--- KFS-1 KERNEL STARTED ---\n");
    printk("Testing printf features:\n");
    printk("Char: %c, String: %s\n", 'A', "Hello 42");
    int answer = 42;
    printk("Decimal: %d\n", answer);
    printk("Negative: %d\n", -123);
    printk("Hex (VGA Addr): %x\n", 0xB8000);
    printk("\nStarting Scroll Test in 3... 2... 1...\n");
    
    for(int i = 0; i < 100000000; i++) __asm__ volatile("nop");

    for (int i = 1; i <= 30; i++) {
        printk("Log Entry Number: %d (System is running...)\n", i);
        // Biraz yavaşlat ki aktığını gör
        for(volatile int j=0; j<100000000; j++); 
    }

    printk("--- TEST FINISHED ---");
}
