#ifndef KERNEL_H
#define KERNEL_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned int size_t;

#define NULL ((void*)0)

#define va_list __builtin_va_list
#define va_start(v,l) __builtin_va_start(v,l)
#define va_end(v) __builtin_va_end(v)
#define va_arg(v,l) __builtin_va_arg(v,l)

/* VGA Sabitleri */
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

/* Ekran Sabitleri */
#define NUM_SCREENS 5
#define CMD_BUFFER_SIZE 256

/* Renkler */
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

/* Port I/O */
void outb(uint16_t port, uint8_t val);
uint8_t inb(uint16_t port);

/* Cursor */
void update_cursor(int x, int y);
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end);

/* String & Memory */
size_t strlen(const char* str);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
void* memset(void* ptr, int value, size_t num);
void* memcpy(void* destination, const void* source, size_t num);

/* VGA helpers */
uint16_t vga_entry(unsigned char uc, uint8_t color);
uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg);

/* Terminal */
void terminal_initialize(void);
void terminal_setcolor(uint8_t color);
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y);
void terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);
void terminal_scroll(void);
void terminal_switch_screen(int screen_index);
void terminal_clear(void);

/* Output */
void itoa(int value, char* str, int base);
void printk(const char* format, ...);

/* Command system */
void process_command(const char* cmd);
void print_prompt(void);

/* Log & Test */
void kernel_log(const char* msg);
void run_tests(void);

/* Keyboard & Screen interface */
typedef struct {
    uint16_t buffer[VGA_WIDTH * VGA_HEIGHT];
    size_t row;
    size_t column;
    uint8_t color;
    char cmd_buffer[CMD_BUFFER_SIZE];
    int cmd_len;
    size_t input_start_row;
    size_t input_start_col;
} Screen;

extern Screen screens[NUM_SCREENS];
extern int current_screen;

#endif