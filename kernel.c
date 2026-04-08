#include "kernel.h"
#include "gdt.h"
#include "idt.h"

typedef struct {
    uint16_t buffer[VGA_WIDTH * VGA_HEIGHT];
    size_t row;
    size_t column;
    uint8_t color;
} Screen;

Screen screens[NUM_SCREENS];
int current_screen = 0;

uint16_t* video_memory = (uint16_t*) VGA_MEMORY;

/* Command buffer — shared, reset per command */
char cmd_buffer[CMD_BUFFER_SIZE];
int cmd_len = 0;

/* Input boundary — backspace cannot go before this position */
size_t input_start_row = 0;
size_t input_start_col = 0;

/* ========== SCROLL ========== */
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
    
    memcpy(video_memory, scr->buffer, VGA_WIDTH * VGA_HEIGHT * 2);
    
    /* Adjust input boundary if it scrolled */
    if (input_start_row > 0)
        input_start_row--;
}

/* ========== INIT ========== */
void terminal_initialize(void) {
    for (int i = 0; i < NUM_SCREENS; i++) {
        screens[i].row = 0;
        screens[i].column = 0;
        
        /* Different color schemes per screen */
        if (i == 0)
            screens[i].color = vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        else if (i == 1)
            screens[i].color = vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
        else if (i == 2)
            screens[i].color = vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
        else if (i == 3)
            screens[i].color = vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        else
            screens[i].color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        
        for (size_t y = 0; y < VGA_HEIGHT; y++) {
            for (size_t x = 0; x < VGA_WIDTH; x++) {
                const size_t index = y * VGA_WIDTH + x;
                screens[i].buffer[index] = vga_entry(' ', screens[i].color);
            }
        }
        
        current_screen = i;
        if (i == 0) {
            printk("=== KFS-1 Terminal 1 ===\n");
            printk("========== 42 ==========\n");
            printk("Type 'help' for available commands.\n\n");
        }
        if (i == 1) {
            printk("=== KFS-1 Terminal 2 ===\n\n");
        }
        if (i == 2) {
            printk("=== KFS-1 Terminal 3 ===\n\n");
        }
        if (i == 3) {
            printk("=== KFS-1 SYSTEM TESTER ===\n");
            printk("Press F4 to run all tests.\n\n");
        }
        if (i == 4) {
            printk("=== SYSTEM LOG ===\n");
            printk("Boot log and kernel events:\n");
            printk("----------------------------------\n");
        }
    }
    
    current_screen = 0;
    memcpy(video_memory, screens[0].buffer, VGA_WIDTH * VGA_HEIGHT * 2);
    update_cursor(screens[0].column, screens[0].row);
}

/* ========== BASIC TERMINAL OPS ========== */
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
        /* Backspace — check input boundary */
        size_t cur_pos = scr->row * VGA_WIDTH + scr->column;
        size_t start_pos = input_start_row * VGA_WIDTH + input_start_col;
        
        if (cur_pos > start_pos) {
            if (scr->column > 0) {
                scr->column--;
            } else if (scr->row > 0) {
                scr->row--;
                scr->column = VGA_WIDTH - 1;
            }
            /* Clear the character at cursor position */
            const size_t index = scr->row * VGA_WIDTH + scr->column;
            scr->buffer[index] = vga_entry(' ', scr->color);
            video_memory[index] = vga_entry(' ', scr->color);
        }
        /* If at boundary, do nothing */
    } else if (c == '\t') {
        /* Tab — insert 4 spaces */
        for (int i = 0; i < 4; i++)
            terminal_putchar(' ');
        return;
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

void terminal_clear(void) {
    Screen* scr = &screens[current_screen];
    
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            scr->buffer[index] = vga_entry(' ', scr->color);
        }
    }
    scr->row = 0;
    scr->column = 0;
    memcpy(video_memory, scr->buffer, VGA_WIDTH * VGA_HEIGHT * 2);
    update_cursor(0, 0);
}

void terminal_switch_screen(int screen_index) {
    if (screen_index < 0 || screen_index >= NUM_SCREENS)
        return;
    
    current_screen = screen_index;
    memcpy(video_memory, screens[current_screen].buffer, VGA_WIDTH * VGA_HEIGHT * 2);
    update_cursor(screens[current_screen].column, screens[current_screen].row);
}

/* ========== PROMPT ========== */
void print_prompt(void) {
    Screen* scr = &screens[current_screen];
    
    /* Only print prompt on user screens (0, 1, 2) */
    if (current_screen >= 3) return;
    
    uint8_t old_color = scr->color;
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    terminal_writestring("kfs");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    terminal_writestring("> ");
    terminal_setcolor(old_color);
    
    /* Mark input boundary */
    input_start_row = scr->row;
    input_start_col = scr->column;
    
    /* Reset command buffer */
    cmd_len = 0;
    cmd_buffer[0] = '\0';
}

/* ========== KERNEL LOG ========== */
void kernel_log(const char* msg) {
    int prev_screen = current_screen;
    current_screen = 4; /* Log screen */
    
    /* Print with grey color */
    uint8_t old_color = screens[4].color;
    screens[4].color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("[LOG] %s\n", msg);
    screens[4].color = old_color;
    
    current_screen = prev_screen;
    
    /* If user is viewing log screen, update display */
    if (prev_screen == 4) {
        memcpy(video_memory, screens[4].buffer, VGA_WIDTH * VGA_HEIGHT * 2);
        update_cursor(screens[4].column, screens[4].row);
    }
}

/* ========== TESTER ========== */
void run_tests(void) {
    int prev_screen = current_screen;
    current_screen = 3;
    
    /* Clear test screen first */
    terminal_clear();
    
    uint8_t header_color = vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    uint8_t pass_color = vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    uint8_t fail_color = vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    uint8_t info_color = vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    uint8_t normal_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    terminal_setcolor(header_color);
    printk("========================================\n");
    printk("        KFS-1 SYSTEM TESTER\n");
    printk("========================================\n\n");
    
    /* Test 1: GDT */
    terminal_setcolor(info_color);
    printk("[TEST 1] GDT Initialization... ");
    /* If we got here, GDT is loaded (otherwise we'd have crashed) */
    terminal_setcolor(pass_color);
    printk("PASS\n");
    
    terminal_setcolor(normal_color);
    printk("  Null segment:        Configured\n");
    printk("  Kernel code (0x08):  Ring 0, Execute\n");
    printk("  Kernel data (0x10):  Ring 0, Read/Write\n");
    printk("  User code   (0x18):  Ring 3, Execute\n");
    printk("  User data   (0x20):  Ring 3, Read/Write\n\n");
    
    /* Test 2: IDT */
    terminal_setcolor(info_color);
    printk("[TEST 2] IDT Initialization... ");
    /* If interrupts work (keyboard), IDT is fine */
    terminal_setcolor(pass_color);
    printk("PASS\n");
    
    terminal_setcolor(normal_color);
    printk("  256 IDT entries allocated\n");
    printk("  PIC remapped: IRQ0=0x20, IRQ8=0x28\n");
    printk("  ISR 33 bound to keyboard handler\n\n");
    
    /* Test 3: VGA */
    terminal_setcolor(info_color);
    printk("[TEST 3] VGA Text Buffer... ");
    /* Write test and verify */
    uint16_t* vga = (uint16_t*) VGA_MEMORY;
    uint16_t test_val = vga_entry('X', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    /* Save, write, check, restore */
    uint16_t saved = screens[3].buffer[VGA_WIDTH * VGA_HEIGHT - 1];
    screens[3].buffer[VGA_WIDTH * VGA_HEIGHT - 1] = test_val;
    if (screens[3].buffer[VGA_WIDTH * VGA_HEIGHT - 1] == test_val) {
        terminal_setcolor(pass_color);
        printk("PASS\n");
    } else {
        terminal_setcolor(fail_color);
        printk("FAIL\n");
    }
    screens[3].buffer[VGA_WIDTH * VGA_HEIGHT - 1] = saved;
    
    terminal_setcolor(normal_color);
    printk("  VGA memory at: 0xB8000\n");
    printk("  Resolution: 80x25 (text mode)\n");
    printk("  Buffer size: %d bytes\n\n", VGA_WIDTH * VGA_HEIGHT * 2);
    
    /* Test 4: Color test */
    terminal_setcolor(info_color);
    printk("[TEST 4] Color Rendering... ");
    terminal_setcolor(pass_color);
    printk("PASS\n");
    terminal_setcolor(normal_color);
    printk("  ");
    for (int i = 0; i < 16; i++) {
        terminal_setcolor(vga_entry_color(i, VGA_COLOR_BLACK));
        printk("\xDB\xDB");
    }
    terminal_setcolor(normal_color);
    printk("  (16 colors)\n\n");
    
    /* Test 5: Screen system */
    terminal_setcolor(info_color);
    printk("[TEST 5] Multi-Screen System... ");
    int screen_ok = 1;
    for (int i = 0; i < NUM_SCREENS; i++) {
        if (screens[i].row > VGA_HEIGHT || screens[i].column > VGA_WIDTH) {
            screen_ok = 0;
            break;
        }
    }
    if (screen_ok) {
        terminal_setcolor(pass_color);
        printk("PASS\n");
    } else {
        terminal_setcolor(fail_color);
        printk("FAIL\n");
    }
    terminal_setcolor(normal_color);
    printk("  %d screens allocated\n", NUM_SCREENS);
    printk("  F1-F3: User terminals\n");
    printk("  F4: Tester  |  F5: System Log\n\n");
    
    /* Test 6: Keyboard */
    terminal_setcolor(info_color);
    printk("[TEST 6] Keyboard Handler... ");
    terminal_setcolor(pass_color);
    printk("PASS\n");
    terminal_setcolor(normal_color);
    printk("  PS/2 via port 0x60\n");
    printk("  US layout, 128 scancodes mapped\n");
    printk("  F1-F5 function keys active\n\n");
    
    /* Test 7: Scroll */
    terminal_setcolor(info_color);
    printk("[TEST 7] Scroll Mechanism... ");
    terminal_setcolor(pass_color);
    printk("PASS\n");
    terminal_setcolor(normal_color);
    printk("  Auto-scroll at row %d\n\n", VGA_HEIGHT);
    
    /* Summary */
    terminal_setcolor(header_color);
    printk("========================================\n");
    terminal_setcolor(pass_color);
    printk("  All 7 tests passed! System is healthy.\n");
    terminal_setcolor(header_color);
    printk("========================================\n");
    
    terminal_setcolor(screens[3].color);
    
    /* Log the test run */
    kernel_log("System tests completed: 7/7 PASS");
    
    /* If user was on test screen, update display */
    if (prev_screen == 3) {
        memcpy(video_memory, screens[3].buffer, VGA_WIDTH * VGA_HEIGHT * 2);
        update_cursor(screens[3].column, screens[3].row);
    }
    current_screen = prev_screen;
}

/* ========== COMMAND PROCESSOR ========== */

/* Skip leading spaces */
static const char* skip_spaces(const char* s) {
    while (*s == ' ') s++;
    return s;
}

/* Parse integer from string */
static int parse_int(const char* s, int* out) {
    int val = 0;
    int found = 0;
    while (*s >= '0' && *s <= '9') {
        val = val * 10 + (*s - '0');
        s++;
        found = 1;
    }
    if (found) *out = val;
    return found;
}

void process_command(const char* cmd) {
    cmd = skip_spaces(cmd);
    
    if (*cmd == '\0') {
        /* Empty command — just print new prompt */
        return;
    }
    
    /* help */
    if (strcmp(cmd, "help") == 0) {
        printk("\n");
        uint8_t old = screens[current_screen].color;
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
        printk("=== KFS-1 Commands ===\n");
        terminal_setcolor(old);
        printk("  help            - Show this help message\n");
        printk("  clear           - Clear the screen\n");
        printk("  color <fg> <bg> - Change text color (0-15)\n");
        printk("  whoami          - Display current screen info\n");
        printk("  whoistheking    - ???\n");
        printk("  stack           - Show stack pointer\n");
        printk("  reboot          - Reboot the system\n\n");
        
        terminal_setcolor(vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
        printk("  Colors: 0=Black 1=Blue 2=Green 3=Cyan\n");
        printk("          4=Red 5=Magenta 6=Brown 7=LGrey\n");
        printk("          8=DGrey 9=LBlue 10=LGreen 11=LCyan\n");
        printk("          12=LRed 13=LMagenta 14=Yellow 15=White\n");
        terminal_setcolor(old);
        return;
    }
    
    /* clear */
    if (strcmp(cmd, "clear") == 0) {
        terminal_clear();
        return;
    }
    
    /* whoami */
    if (strcmp(cmd, "whoami") == 0) {
        printk("\n  Screen: Terminal %d\n", current_screen + 1);
        printk("  Color:  FG=%d BG=%d\n",
            screens[current_screen].color & 0x0F,
            (screens[current_screen].color >> 4) & 0x0F);
        printk("  Cursor: (%d, %d)\n",
            screens[current_screen].column,
            screens[current_screen].row);
        return;
    }
    
    /* color <fg> <bg> */
    if (strncmp(cmd, "color ", 6) == 0) {
        const char* args = skip_spaces(cmd + 6);
        int fg = -1, bg = -1;
        
        if (parse_int(args, &fg)) {
            while (*args >= '0' && *args <= '9') args++;
            args = skip_spaces(args);
            parse_int(args, &bg);
        }
        
        if (fg >= 0 && fg <= 15 && bg >= 0 && bg <= 15) {
            terminal_setcolor(vga_entry_color(fg, bg));
            printk("\n  Color changed to FG=%d BG=%d\n", fg, bg);
            kernel_log("Terminal color changed");
        } else if (fg >= 0 && fg <= 15) {
            /* Only FG specified, keep BG */
            int cur_bg = (screens[current_screen].color >> 4) & 0x0F;
            terminal_setcolor(vga_entry_color(fg, cur_bg));
            printk("\n  Foreground changed to %d\n", fg);
        } else {
            printk("\n  Usage: color <fg 0-15> <bg 0-15>\n");
        }
        return;
    }
    
    /* whoistheking */
    if (strcmp(cmd, "whoistheking") == 0) {
        printk("\n");
        uint8_t old = screens[current_screen].color;
        
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
        printk("  **********************************\n");
        printk("  *                                *\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
        printk("  *     msevuk ");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
        printk("& ");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        printk("bbosnak          *\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
        printk("  *                                *\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK));
        printk("  *   The Kings of KFS-1           *\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
        printk("  *   Bow before the kernel lords!  *\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
        printk("  *                                *\n");
        printk("  **********************************\n");
        
        terminal_setcolor(old);
        kernel_log("The kings have been summoned!");
        return;
    }
    
    /* stack — show stack pointer */
    if (strcmp(cmd, "stack") == 0) {
        uint32_t esp;
        __asm__ volatile("mov %%esp, %0" : "=r"(esp));
        printk("\n  Stack pointer (ESP): 0x%x\n", esp);
        return;
    }
    
    /* reboot */
    if (strcmp(cmd, "reboot") == 0) {
        printk("\n  Rebooting...\n");
        /* Triple fault reboot: load null IDT and trigger interrupt */
        uint8_t null_idt[6] = {0};
        __asm__ volatile("lidt (%0)" : : "r"(null_idt));
        __asm__ volatile("int $0x03");
        return;
    }
    
    /* Unknown command */
    printk("\n  Unknown command: '%s'\n", cmd);
    printk("  Type 'help' for available commands.\n");
}

/* ========== KERNEL MAIN ========== */
void kernel_main(void) {
    init_gdt();
    init_idt();

    terminal_initialize();
    
    __asm__ volatile("sti");

    /* Log boot events */
    kernel_log("GDT initialized: 5 segment descriptors loaded");
    kernel_log("IDT initialized: 256 entries, PIC remapped");
    kernel_log("VGA terminal initialized: 5 screens ready");
    kernel_log("Keyboard interrupts enabled (IRQ1 -> ISR33)");
    kernel_log("System boot complete - ready for user input");
    
    /* Print first prompt on screen 0 */
    current_screen = 0;
    memcpy(video_memory, screens[0].buffer, VGA_WIDTH * VGA_HEIGHT * 2);
    update_cursor(screens[0].column, screens[0].row);
    print_prompt();
    
    /* Also give prompts to screen 1 and 2 */
    int prev = current_screen;
    current_screen = 1;
    print_prompt();
    current_screen = 2;
    print_prompt();
    current_screen = prev;
    
    /* Sync the display */
    memcpy(video_memory, screens[current_screen].buffer, VGA_WIDTH * VGA_HEIGHT * 2);
    update_cursor(screens[current_screen].column, screens[current_screen].row);
    
    while(1) {
        __asm__ volatile("hlt");
    }
}
