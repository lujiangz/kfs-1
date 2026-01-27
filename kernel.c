#include "kernel.h"
#include "gdt.h"
#include "idt.h"

#define NUM_SCREENS 3

typedef struct {
    uint16_t buffer[VGA_WIDTH * VGA_HEIGHT];
    size_t row;
    size_t column;
    uint8_t color;
} Screen;

Screen screens[NUM_SCREENS];
int current_screen = 0;

uint16_t* video_memory = (uint16_t*) VGA_MEMORY;

/* Ekranı bir satır yukarı kaydırır */
void terminal_scroll(void) {
    Screen* scr = &screens[current_screen];
    
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            scr->buffer[y * VGA_WIDTH + x] = scr->buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        scr->buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', scr->color);
    }
    
    /* VGA belleğini güncelle */
    memcpy(video_memory, scr->buffer, VGA_WIDTH * VGA_HEIGHT * 2);
}

void terminal_initialize(void) {
    for (int i = 0; i < NUM_SCREENS; i++) {
        screens[i].row = 0;
        screens[i].column = 0;
        screens[i].color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        
        for (size_t y = 0; y < VGA_HEIGHT; y++) {
            for (size_t x = 0; x < VGA_WIDTH; x++) {
                const size_t index = y * VGA_WIDTH + x;
                screens[i].buffer[index] = vga_entry(' ', screens[i].color);
            }
        }
        
        // Write Screen ID to buffer directly for visibility
        current_screen = i; // Temporarily switch context to write
        if (i == 0) printk("Screen 0: System Log\n");
        if (i == 1) printk("Screen 1: Workspace A\n");
        if (i == 2) printk("Screen 2: Workspace B\n");
    }
    
    current_screen = 0;
    memcpy(video_memory, screens[0].buffer, VGA_WIDTH * VGA_HEIGHT * 2);
    update_cursor(screens[0].column, screens[0].row);
}

void terminal_setcolor(uint8_t color) {
    screens[current_screen].color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    screens[current_screen].buffer[index] = vga_entry(c, color);
    video_memory[index] = vga_entry(c, color);
}

void terminal_putchar(char c) {
    Screen* scr = &screens[current_screen];
    
    if (c == '\n') {
        scr->column = 0;
        scr->row++;
    } else if (c == '\b') {
        if (scr->column > 0) {
            scr->column--;
        } else if (scr->row > 0) {
            scr->row--;
            scr->column = VGA_WIDTH - 1;
        }
        terminal_putentryat(' ', scr->color, scr->column, scr->row);
    } else {
        terminal_putentryat(c, scr->color, scr->column, scr->row);
        if (++scr->column == VGA_WIDTH) {
            scr->column = 0;
            scr->row++;
        }
    }

    if (scr->row == VGA_HEIGHT) {
        terminal_scroll();
        scr->row = VGA_HEIGHT - 1;
    }
    update_cursor(scr->column, scr->row);
}

void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++)
        terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) {
    terminal_write(data, strlen(data));
}

void terminal_switch_screen(int screen_index) {
    if (screen_index < 0 || screen_index >= NUM_SCREENS)
        return;
    
    current_screen = screen_index;
    memcpy(video_memory, screens[current_screen].buffer, VGA_WIDTH * VGA_HEIGHT * 2);
    update_cursor(screens[current_screen].column, screens[current_screen].row);
}

void kernel_main(void) {
    init_gdt();
    init_idt();

    terminal_initialize(); // This now initializes all screens with text
    
    __asm__ volatile("sti");

    printk("\nKeyboard is active. Type something:\n");
    printk("Commands: F1 (Log), F2 (Work A), F3 (Work B)\n");
    
    // Infinite loop to keep the kernel running and listening to interrupts
    while(1) {
        __asm__ volatile("hlt");
    }
}
