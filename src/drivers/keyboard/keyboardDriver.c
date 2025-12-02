#include "drivers/keyboard/keyboardDriver.h"

#define BUFFER_SIZE 256

static int scanecode = 0;
static char key_buffer[BUFFER_SIZE];
static int buffer_count = 0;
static uint8_t shift_pressed = 0;
static uint8_t caps_lock = 0;

Scancode_entity us_keymap[] = {
    {0x00, 0, 0, 0, 0, "Error/Overrun"},
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
    {0x12, 'e', 'E', '€', 'E', "E"},
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
    {0x3B, 0, 0, 0, 0, "F1"},
    {0x3C, 0, 0, 0, 0, "F2"},
    {0x3D, 0, 0, 0, 0, "F3"},
    {0x3E, 0, 0, 0, 0, "F4"},
    {0x3F, 0, 0, 0, 0, "F5"},
    {0x40, 0, 0, 0, 0, "F6"},
    {0x41, 0, 0, 0, 0, "F7"},
    {0x42, 0, 0, 0, 0, "F8"},
    {0x43, 0, 0, 0, 0, "F9"},
    {0x44, 0, 0, 0, 0, "F10"},
    
    {0x45, 0, 0, 0, 0, "NumLock"},
    {0x46, 0, 0, 0, 0, "ScrollLock"},
    {0x47, '7', 0, 0, '7', "Keypad 7/Home"},
    {0x48, '8', 0, 0, '8', "Keypad 8/Up"},
    {0x49, '9', 0, 0, '9', "Keypad 9/PgUp"},
    {0x4A, '-', '-', '-', '-', "Keypad -"},
    {0x4B, '4', 0, 0, '4', "Keypad 4/Left"},
    {0x4C, '5', 0, 0, '5', "Keypad 5"},
    {0x4D, '6', 0, 0, '6', "Keypad 6/Right"},
    {0x4E, '+', '+', '+', '+', "Keypad +"},
    {0x4F, '1', 0, 0, '1', "Keypad 1/End"},
    {0x50, '2', 0, 0, '2', "Keypad 2/Down"},
    {0x51, '3', 0, 0, '3', "Keypad 3/PgDn"},
    {0x52, '0', 0, 0, '0', "Keypad 0/Ins"},
    {0x53, '.', 0, 0, '.', "Keypad ./Del"},
    {0x56, '\\', '|', 0, '\\', "Non-US \\ and |"},
    {0x57, 0, 0, 0, 0, "F11"},
    {0x58, 0, 0, 0, 0, "F12"},
    
    {0x5B, 0, 0, 0, 0, "Left Windows"},  
    {0x5C, 0, 0, 0, 0, "Right Windows"}, 
    {0x5D, 0, 0, 0, 0, "Application"},   
    {0x5E, 0, 0, 0, 0, "ACPI Power"},    
    {0x5F, 0, 0, 0, 0, "ACPI Sleep"},    
    {0x63, 0, 0, 0, 0, "ACPI Wake"},     
    
    {0xE0, 0, 0, 0, 0, "Extended Prefix"},

    {0xAA, 0, 0, 0, 0, "Keyboard Self Test Pass"},
    {0xEE, 0, 0, 0, 0, "Echo"},
    {0xFA, 0, 0, 0, 0, "Acknowledge"},
    {0xFC, 0, 0, 0, 0, "Self Test Failed"},
    {0xFD, 0, 0, 0, 0, "Diagnostic Failure"},
    {0xFE, 0, 0, 0, 0, "Resend Request"},
    {0xFF, 0, 0, 0, 0, "Key Error/Overrun"},
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

static char get_letter(char lower, char upper, uint8_t shift, uint8_t caps) {
    if (shift) return upper;
    if (caps) return upper;
    return lower;
}

static char scancode_to_char(uint8_t scancode) {
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
    scanecode = inb(0x60);
    
    uint8_t is_release = (scanecode & 0x80);
    uint8_t make_code = scanecode & 0x7F;
    
    if (make_code == 0x2A || make_code == 0x36) {
        shift_pressed = !is_release;
    } else if (make_code == 0x3A && !is_release) {
        caps_lock = !caps_lock;
    }
    
    if (!is_release) {
        char c = scancode_to_char(make_code);
        if (c != 0) {
            add_to_buffer(c);
        }
    }
    
    outb(0x20, 0x20);
}

int get_scancode(void) {
    return scanecode;
}

Scancode_entity* get_key(int scancode) {
    int size = sizeof(us_keymap) / sizeof(Scancode_entity);
    for (int i = 0; i < size; i++) {
        if (us_keymap[i].scancode == scancode) {
            return &us_keymap[i];
        }
    }
    return NULL;
}

char getch(void) {
    while (is_buffer_empty()) {
        __asm__ volatile("pause");
    }
    return get_from_buffer();
}

void gets(char* buffer, int max_len) {
    int pos = 0;
    
    while (1) {
        char c = getch();
        
        if (c == '\n') {
            buffer[pos] = '\0';
            return;
        }
        else if (c == '\b') {
            if (pos > 0) {
                pos--;
            }
        }
        else if (c == 0x03) {
            buffer[0] = '\0';
            return;
        }
        else if (pos < max_len - 1) {
            buffer[pos] = c;
            pos++;
        }
    }
}