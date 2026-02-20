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

/* String Karsilastirma */
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}


/* VGA Yardımcıları */
uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

/* Integer to ASCII */
void itoa(int value, char* str, int base) {
    char *ptr = str, *ptr1 = str, tmp_char;
    int tmp_value;

    if (base < 2 || base > 36) {
        *str = '\0';
        return;
    }

    if (value < 0 && base == 10) {
        *ptr++ = '-';
        str++; 
        value = -value;
    }

    unsigned int num = (unsigned int)value;
    if (base == 10 && value < 0) {
        num = (unsigned int)(-value);
    } else {
        num = (unsigned int)value;
    }

    if (value == 0) {
        *ptr++ = '0';
        *ptr = '\0';
        return;
    }

    while (num != 0) {
        tmp_value = num % base;
        num /= base;
        if (tmp_value > 9)
            *ptr++ = (tmp_value - 10) + 'a';
        else
            *ptr++ = tmp_value + '0';
    }

    *ptr-- = '\0';
    
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
}

void printk(const char* format, ...) {
    va_list args;
    va_start(args, format);

    while (*format != '\0') {
        if (*format == '%') {
            format++;
            if (*format == 'c') {
                char c = (char)va_arg(args, int);
                terminal_putchar(c);
            } else if (*format == 's') {
                char* s = va_arg(args, char*);
                terminal_writestring(s);
            } else if (*format == 'd' || *format == 'i') {
                int i = va_arg(args, int);
                char buffer[32];
                itoa(i, buffer, 10);
                terminal_writestring(buffer);
            } else if (*format == 'x') {
                int i = va_arg(args, int);
                char buffer[32];
                itoa(i, buffer, 16);
                terminal_writestring("0x");
                terminal_writestring(buffer);
            } else if (*format == '%') {
                terminal_putchar('%');
            }
        } else {
            terminal_putchar(*format);
        }
        format++;
    }

    va_end(args);
}

/* Bellek İşlemleri */
void* memset(void* ptr, int value, size_t num) {
    unsigned char* p = (unsigned char*)ptr;
    while (num--)
        *p++ = (unsigned char)value;
    return ptr;
}

void* memcpy(void* destination, const void* source, size_t num) {
    unsigned char* dest = (unsigned char*)destination;
    const unsigned char* src = (const unsigned char*)source;
    while (num--)
        *dest++ = *src++;
    return destination;
}
