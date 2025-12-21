#include "kernel.h"

/* Port Veri Gönderme */
void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "nd"(port) );
}

/* Port Veri Okuma */
uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "nd"(port) );
    return ret;
}

void update_cursor(int x, int y) {
    uint16_t pos = y * VGA_WIDTH + x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t) (pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

void enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

/* VGA Yardımcıları */
uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

void k_string_reverse(char* str, int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

void k_int_to_string(int n, char* buffer, int base) {
    int i = 0;
    int is_negative = 0;

    if (n == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return;
    }

    if (n < 0 && base == 10) {
        is_negative = 1;
        n = -n;
    }

    const char* digits = "0123456789ABCDEF";

    while (n != 0) {
        int remainder = n % base;
        buffer[i++] = digits[remainder];
        n = n / base;
    }

    if (is_negative) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0';

    k_string_reverse(buffer, i);
}

void printk(const char* format, ...) {
    va_list args;
    va_start(args, format);

    char num_buffer[32]; // Buffer for number conversions

    for (size_t i = 0; format[i] != '\0'; i++) {
        
        if (format[i] != '%') {
            terminal_putchar(format[i]);
            continue;
        }

        i++; // Skip '%'

        switch (format[i]) {
            case 'c': {
                char c = (char)va_arg(args, int);
                terminal_putchar(c);
                break;
            }
            case 's': {
                const char* s = va_arg(args, const char*);
                if (!s) s = "(null)";
                terminal_writestring(s);
                break;
            }
            case 'd': {
                int val = va_arg(args, int);
                k_int_to_string(val, num_buffer, 10);
                terminal_writestring(num_buffer);
                break;
            }
            case 'x': {
                unsigned int val = va_arg(args, unsigned int);
                terminal_writestring("0x");
                k_int_to_string(val, num_buffer, 16);
                terminal_writestring(num_buffer);
                break;
            }
            default: {
                terminal_putchar('%');
                terminal_putchar(format[i]);
                break;
            }
        }
    }

    va_end(args);
}