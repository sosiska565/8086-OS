#include "drivers/keyboard/keyboardDriver.h"
#include "drivers/vga/vga.h"
#include "drivers/io/io.h"

#define BUFFER_SIZE 256

static uint8_t shift_pressed = 0;
static uint8_t caps_lock = 0;

static char key_buffer[BUFFER_SIZE];
static int buffer_count = 0;

static uint8_t scancode_buffer[BUFFER_SIZE];
static int scancode_count = 0;

Scancode_entity us_keymap[] = {
    {0x00, 0, 0, 0, 0, "Error"},
    {0x01, 0, 0, 0, 0, "ESC"},
    {0x02, '1', '!', '~', '1', "1"},
    {0x03, '2', '@', '`', '2', "2"},
    {0x04, '3', '#', 0, '3', "3"},
    {0x05, '4', '$', 0, '4', "4"},
    {0x06, '5', '%', 0, '5', "5"},
    {0x07, '6', '^', 0, '6', "6"},
    {0x08, '7', '&', 0, '7', "7"},
    {0x09, '8', '*', 0, '8', "8"},
    {0x0A, '9', '(', 0, '9', "9"},
    {0x0B, '0', ')', 0, '0', "0"},
    {0x0C, '-', '_', 0, '-', "-"},
    {0x0D, '=', '+', 0, '=', "="},
    {0x0E, '\b', '\b', '\b', '\b', "Backspace"},
    {0x0F, '\t', '\t', '\t', '\t', "Tab"},
    
    {0x10, 'q', 'Q', 0, 'Q', "Q"},
    {0x11, 'w', 'W', 0, 'W', "W"},
    {0x12, 'e', 'E', 0, 'E', "E"},
    {0x13, 'r', 'R', 0, 'R', "R"},
    {0x14, 't', 'T', 0, 'T', "T"},
    {0x15, 'y', 'Y', 0, 'Y', "Y"},
    {0x16, 'u', 'U', 0, 'U', "U"},
    {0x17, 'i', 'I', 0, 'I', "I"},
    {0x18, 'o', 'O', 0, 'O', "O"},
    {0x19, 'p', 'P', 0, 'P', "P"},
    {0x1A, '[', '{', 0, '[', "["},
    {0x1B, ']', '}', 0, ']', "]"},
    {0x1C, '\n', '\n', '\n', '\n', "Enter"},
    
    {0x1D, 0, 0, 0, 0, "Left Ctrl"},
    {0x1E, 'a', 'A', 0, 'A', "A"},
    {0x1F, 's', 'S', 0, 'S', "S"},
    {0x20, 'd', 'D', 0, 'D', "D"},
    {0x21, 'f', 'F', 0, 'F', "F"},
    {0x22, 'g', 'G', 0, 'G', "G"},
    {0x23, 'h', 'H', 0, 'H', "H"},
    {0x24, 'j', 'J', 0, 'J', "J"},
    {0x25, 'k', 'K', 0, 'K', "K"},
    {0x26, 'l', 'L', 0, 'L', "L"},
    {0x27, ';', ':', 0, ';', ";"},
    {0x28, '\'', '"', 0, '\'', "'"},
    {0x29, '`', '~', 0, '`', "`"},
    {0x2A, 0, 0, 0, 0, "Left Shift"},
    
    {0x2B, '\\', '|', 0, '\\', "\\"},
    {0x2C, 'z', 'Z', 0, 'Z', "Z"},
    {0x2D, 'x', 'X', 0, 'X', "X"},
    {0x2E, 'c', 'C', 0, 'C', "C"},
    {0x2F, 'v', 'V', 0, 'V', "V"},
    {0x30, 'b', 'B', 0, 'B', "B"},
    {0x31, 'n', 'N', 0, 'N', "N"},
    {0x32, 'm', 'M', 0, 'M', "M"},
    {0x33, ',', '<', 0, ',', ","},
    {0x34, '.', '>', 0, '.', "."},
    {0x35, '/', '?', 0, '/', "/"},
    {0x36, 0, 0, 0, 0, "Right Shift"},
    {0x37, '*', '*', '*', '*', "Keypad *"},
    {0x38, 0, 0, 0, 0, "Left Alt"},
    {0x39, ' ', ' ', ' ', ' ', "Space"},
    
    {0x3A, 0, 0, 0, 0, "CapsLock"},
};

static void add_to_buffer(char c) {
    if (buffer_count < BUFFER_SIZE) {
        key_buffer[buffer_count] = c;
        buffer_count++;
    }
}

static int is_buffer_empty(void) {
    return buffer_count == 0;
}

static char get_from_buffer(void) {
    if (buffer_count > 0) {
        char c = key_buffer[0];
        for (int i = 1; i < buffer_count; i++) {
            key_buffer[i-1] = key_buffer[i];
        }
        buffer_count--;
        return c;
    }
    return 0;
}

static void add_scancode_to_buffer(uint8_t code) {
    if (scancode_count < BUFFER_SIZE) {
        scancode_buffer[scancode_count] = code;
        scancode_count++;
    }
}

static int is_scancode_buffer_empty(void) {
    return scancode_count == 0;
}

static uint8_t get_scancode_from_buffer(void) {
    if (scancode_count > 0) {
        uint8_t code = scancode_buffer[0];
        for (int i = 1; i < scancode_count; i++) {
            scancode_buffer[i-1] = scancode_buffer[i];
        }
        scancode_count--;
        return code;
    }
    return 0;
}

static char get_letter(char lower, char upper, uint8_t shift, uint8_t caps) {
    if (shift) return upper;
    if (caps) return upper;
    return lower;
}

char scancode_to_char(uint8_t scancode) {
    if (scancode & 0x80) return 0;
    
    switch(scancode) {
        case 0x02: return shift_pressed ? '!' : '1';
        case 0x03: return shift_pressed ? '@' : '2';
        case 0x04: return shift_pressed ? '#' : '3';
        case 0x05: return shift_pressed ? '$' : '4';
        case 0x06: return shift_pressed ? '%' : '5';
        case 0x07: return shift_pressed ? '^' : '6';
        case 0x08: return shift_pressed ? '&' : '7';
        case 0x09: return shift_pressed ? '*' : '8';
        case 0x0A: return shift_pressed ? '(' : '9';
        case 0x0B: return shift_pressed ? ')' : '0';
        case 0x0C: return shift_pressed ? '_' : '-';
        case 0x0D: return shift_pressed ? '+' : '=';
        case 0x0E: return '\b';
        case 0x0F: return '\t';
        case 0x10: return get_letter('q', 'Q', shift_pressed, caps_lock);
        case 0x11: return get_letter('w', 'W', shift_pressed, caps_lock);
        case 0x12: return get_letter('e', 'E', shift_pressed, caps_lock);
        case 0x13: return get_letter('r', 'R', shift_pressed, caps_lock);
        case 0x14: return get_letter('t', 'T', shift_pressed, caps_lock);
        case 0x15: return get_letter('y', 'Y', shift_pressed, caps_lock);
        case 0x16: return get_letter('u', 'U', shift_pressed, caps_lock);
        case 0x17: return get_letter('i', 'I', shift_pressed, caps_lock);
        case 0x18: return get_letter('o', 'O', shift_pressed, caps_lock);
        case 0x19: return get_letter('p', 'P', shift_pressed, caps_lock);
        case 0x1A: return shift_pressed ? '{' : '[';
        case 0x1B: return shift_pressed ? '}' : ']';
        case 0x1C: return '\n';
        case 0x1E: return get_letter('a', 'A', shift_pressed, caps_lock);
        case 0x1F: return get_letter('s', 'S', shift_pressed, caps_lock);
        case 0x20: return get_letter('d', 'D', shift_pressed, caps_lock);
        case 0x21: return get_letter('f', 'F', shift_pressed, caps_lock);
        case 0x22: return get_letter('g', 'G', shift_pressed, caps_lock);
        case 0x23: return get_letter('h', 'H', shift_pressed, caps_lock);
        case 0x24: return get_letter('j', 'J', shift_pressed, caps_lock);
        case 0x25: return get_letter('k', 'K', shift_pressed, caps_lock);
        case 0x26: return get_letter('l', 'L', shift_pressed, caps_lock);
        case 0x27: return shift_pressed ? ':' : ';';
        case 0x28: return shift_pressed ? '"' : '\'';
        case 0x29: return shift_pressed ? '~' : '`';
        case 0x2B: return shift_pressed ? '|' : '\\';
        case 0x2C: return get_letter('z', 'Z', shift_pressed, caps_lock);
        case 0x2D: return get_letter('x', 'X', shift_pressed, caps_lock);
        case 0x2E: return get_letter('c', 'C', shift_pressed, caps_lock);
        case 0x2F: return get_letter('v', 'V', shift_pressed, caps_lock);
        case 0x30: return get_letter('b', 'B', shift_pressed, caps_lock);
        case 0x31: return get_letter('n', 'N', shift_pressed, caps_lock);
        case 0x32: return get_letter('m', 'M', shift_pressed, caps_lock);
        case 0x33: return shift_pressed ? '<' : ',';
        case 0x34: return shift_pressed ? '>' : '.';
        case 0x35: return shift_pressed ? '?' : '/';
        case 0x39: return ' ';
        default: return 0;
    }
}

void keyboard_handler_c(void) {
    uint8_t scancode = inb(0x60);
    
    uint8_t is_release = (scancode & 0x80);
    uint8_t make_code = scancode & 0x7F;
    
    if (make_code == 0x2A || make_code == 0x36) {
        shift_pressed = !is_release;
    } else if (make_code == 0x3A && !is_release) {
        caps_lock = !caps_lock;
    }
    
    if (!is_release) {
        add_scancode_to_buffer(make_code);

        char c = scancode_to_char(make_code);
        if (c != 0) {
            add_to_buffer(c);
        }
    }
}

char getch(void) {
    __asm__ volatile("sti");
    while (is_buffer_empty()) {
        __asm__ volatile("pause");
    }
    __asm__ volatile("cli");
    char c = get_from_buffer();
    __asm__ volatile("sti");
    return c;
}

uint8_t wait_scancode(void) {
    __asm__ volatile("sti");
    while (is_scancode_buffer_empty()) {
        __asm__ volatile("pause");
    }
    __asm__ volatile("cli");
    uint8_t code = get_scancode_from_buffer();
    __asm__ volatile("sti");
    return code;
}

void gets(char* buffer, int max_len) {
    int pos = 0;
    
    while (1) {
        char c = getch();
        
        if (c == '\n') {
            buffer[pos] = '\0';
            printf("\n");
            return;
        }
        else if (c == '\b') {
            if (pos > 0) {
                pos--;
                printf("\b \b");
            }
        }
        else if (pos < max_len - 1) {
            if(c >= 32 && c <= 126) {
                buffer[pos] = c;
                pos++;
                char str[2] = {c, '\0'};
                printf(str);
            }
        }
    }
}

void keyboard_flush(void) {
    __asm__ volatile("cli");
    
    buffer_count = 0;
    scancode_count = 0;
    
    __asm__ volatile("sti");
}