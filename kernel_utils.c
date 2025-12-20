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

/* İmleç Güncelleme */
void update_cursor(int x, int y) {
    uint16_t pos = y * VGA_WIDTH + x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t) (pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

/* İmleç Açma (Opsiyonel) */
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

/* String Uzunluğu Hesaplama */
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

void print_int(int32_t num) {
    char buffer[12];
    int i = 0;
    int is_negative = 0;
    
    if (num == 0) {
        terminal_putchar('0');
        return;
    }
    
    if (num < 0) {
        is_negative = 1;
        num = -num;
    }
    
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    if (is_negative) {
        terminal_putchar('-');
    }
    
    while (i > 0) {
        terminal_putchar(buffer[--i]);
    }
}

void print_uint(uint32_t num) {
    char buffer[11];
    int i = 0;
    
    if (num == 0) {
        terminal_putchar('0');
        return;
    }
    
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    while (i > 0) {
        terminal_putchar(buffer[--i]);
    }
}

void print_hex(uint32_t num, int uppercase) {
    char buffer[9];
    const char* hex_chars = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    int i = 0;
    
    if (num == 0) {
        terminal_putchar('0');
        return;
    }
    
    while (num > 0) {
        buffer[i++] = hex_chars[num & 0xF];
        num >>= 4;
    }
    
    while (i > 0) {
        terminal_putchar(buffer[--i]);
    }
}

void vprintk(const char* format, va_list args) {
    while (*format) {
        if (*format == '%') {
            format++;
            
            switch (*format) {
                case 's': {
                    const char* str = va_arg(args, const char*);
                    if (str) {
                        terminal_writestring(str);
                    } else {
                        terminal_writestring("(null)");
                    }
                    break;
                }
                case 'c': {
                    char c = (char) va_arg(args, int);
                    terminal_putchar(c);
                    break;
                }
                case 'd':
                case 'i': {
                    int32_t num = va_arg(args, int32_t);
                    print_int(num);
                    break;
                }
                case 'u': {
                    uint32_t num = va_arg(args, uint32_t);
                    print_uint(num);
                    break;
                }
                case 'x': {
                    uint32_t num = va_arg(args, uint32_t);
                    print_hex(num, 0);
                    break;
                }
                case 'X': {
                    uint32_t num = va_arg(args, uint32_t);
                    print_hex(num, 1);
                    break;
                }
                case 'p': {
                    uint32_t ptr = (uint32_t) va_arg(args, void*);
                    terminal_writestring("0x");
                    print_hex(ptr, 0);
                    break;
                }
                case '%': {
                    terminal_putchar('%');
                    break;
                }
                case '\0': {
                    return;
                }
                default: {
                    terminal_putchar('%');
                    terminal_putchar(*format);
                    break;
                }
            }
        } else {
            terminal_putchar(*format);
        }
        format++;
    }
}

void printk(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintk(format, args);
    va_end(args);
}