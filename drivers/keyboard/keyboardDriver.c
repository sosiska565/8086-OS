#include "drivers/keyboard/keyboardDriver.h"
#include "drivers/vga/vga.h"
#include "drivers/io/io.h"
#include "task/task.h"
#include "global.h"
#include "drivers/timer/timer.h" 

#define BUFFER_SIZE 256

static uint8_t shift_pressed = 0;
static uint8_t caps_lock = 0;
static uint8_t ctrl_pressed = 0; 
static uint8_t alt_pressed = 0; 
static uint8_t win_pressed = 0;

static unsigned int key_buffer[BUFFER_SIZE];
static volatile int buffer_count = 0;
static uint8_t scancode_buffer[BUFFER_SIZE];
static volatile int scancode_count = 0;

uint8_t current_layout = 0;

static void add_to_buffer(unsigned int c) { if (buffer_count < BUFFER_SIZE) key_buffer[buffer_count++] = c; }
static int is_buffer_empty(void) { return buffer_count == 0; }
static unsigned int get_from_buffer(void) {
    if (buffer_count > 0) {
        unsigned int c = key_buffer[0];
        for (int i = 1; i < buffer_count; i++) key_buffer[i-1] = key_buffer[i];
        buffer_count--; return c;
    } return 0;
}
static void add_scancode_to_buffer(uint8_t code) { if (scancode_count < BUFFER_SIZE) scancode_buffer[scancode_count++] = code; }
static int is_scancode_buffer_empty(void) { return scancode_count == 0; }
static uint8_t get_scancode_from_buffer(void) {
    if (scancode_count > 0) {
        uint8_t code = scancode_buffer[0];
        for (int i = 1; i < scancode_count; i++) scancode_buffer[i-1] = scancode_buffer[i];
        scancode_count--; return code;
    } return 0;
}

static unsigned int get_letter(unsigned int lower, unsigned int upper, uint8_t shift, uint8_t caps) {
    if (shift || caps) return upper; return lower;
}

unsigned int scancode_to_char(uint8_t scancode) {
    if (scancode & 0x80) return 0;
    switch(scancode) {
        case 0x01: return 27; 
        case 0x02: return shift_pressed ? '!' : '1'; case 0x03: return shift_pressed ? '@' : '2';
        case 0x04: return shift_pressed ? '#' : '3'; case 0x05: return shift_pressed ? '$' : '4';
        case 0x06: return shift_pressed ? '%' : '5'; case 0x07: return shift_pressed ? '^' : '6';
        case 0x08: return shift_pressed ? '&' : '7'; case 0x09: return shift_pressed ? '*' : '8';
        case 0x0A: return shift_pressed ? '(' : '9'; case 0x0B: return shift_pressed ? ')' : '0';
        case 0x0C: return shift_pressed ? '_' : '-'; case 0x0D: return shift_pressed ? '+' : '=';
        case 0x0E: return '\b'; 
        case 0x0F: 
            if (win_pressed) return 23;  
            if (alt_pressed) return 24;  
            return '\t';
        case 0x10: return get_letter('q', 'Q', shift_pressed, caps_lock); case 0x11: return get_letter('w', 'W', shift_pressed, caps_lock);
        case 0x12: return get_letter('e', 'E', shift_pressed, caps_lock); case 0x13: return get_letter('r', 'R', shift_pressed, caps_lock);
        case 0x14: return get_letter('t', 'T', shift_pressed, caps_lock); case 0x15: return get_letter('y', 'Y', shift_pressed, caps_lock);
        case 0x16: return get_letter('u', 'U', shift_pressed, caps_lock); case 0x17: return get_letter('i', 'I', shift_pressed, caps_lock);
        case 0x18: return get_letter('o', 'O', shift_pressed, caps_lock); case 0x19: return get_letter('p', 'P', shift_pressed, caps_lock);
        case 0x1A: return shift_pressed ? '{' : '['; case 0x1B: return shift_pressed ? '}' : ']'; case 0x1C: return '\n';
        case 0x1E: return get_letter('a', 'A', shift_pressed, caps_lock); case 0x1F: return get_letter('s', 'S', shift_pressed, caps_lock);
        case 0x20: return get_letter('d', 'D', shift_pressed, caps_lock); case 0x21: return get_letter('f', 'F', shift_pressed, caps_lock);
        case 0x22: return get_letter('g', 'G', shift_pressed, caps_lock); case 0x23: return get_letter('h', 'H', shift_pressed, caps_lock);
        case 0x24: return get_letter('j', 'J', shift_pressed, caps_lock); case 0x25: return get_letter('k', 'K', shift_pressed, caps_lock);
        case 0x26: return get_letter('l', 'L', shift_pressed, caps_lock); case 0x27: return shift_pressed ? ':' : ';';
        case 0x28: return shift_pressed ? '"' : '\''; case 0x29: return shift_pressed ? '~' : '`'; case 0x2B: return shift_pressed ? '|' : '\\';
        case 0x2C: return get_letter('z', 'Z', shift_pressed, caps_lock); case 0x2D: return get_letter('x', 'X', shift_pressed, caps_lock);
        case 0x2E: return get_letter('c', 'C', shift_pressed, caps_lock); case 0x2F: return get_letter('v', 'V', shift_pressed, caps_lock);
        case 0x30: return get_letter('b', 'B', shift_pressed, caps_lock); case 0x31: return get_letter('n', 'N', shift_pressed, caps_lock);
        case 0x32: return get_letter('m', 'M', shift_pressed, caps_lock); case 0x33: return shift_pressed ? '<' : ',';
        case 0x34: return shift_pressed ? '>' : '.'; case 0x35: return shift_pressed ? '?' : '/'; case 0x39: return ' ';
        case 0x3B: return 21; 
        case 0x3C: return 22; 
        case 0x48: return 17; 
        case 0x4B: return 19; 
        case 0x4D: return 20; 
        case 0x50: return 18; 
        default: return 0;
    }
}

unsigned int scancode_to_char_layout(uint8_t scancode) { return scancode_to_char(scancode); }

void keyboard_flush(void) {
    __asm__ volatile("cli");
    buffer_count = 0;
    scancode_count = 0;
    __asm__ volatile("sti");
}

void keyboard_handler_c(void) {
    uint8_t scancode = inb(0x60);
    uint8_t is_release = (scancode & 0x80);
    uint8_t make_code = scancode & 0x7F;

    if (make_code == 0x2A || make_code == 0x36) shift_pressed = !is_release;
    else if (make_code == 0x3A && !is_release) caps_lock = !caps_lock;
    else if (make_code == 0x1D) ctrl_pressed = !is_release; 
    else if (make_code == 0x38) alt_pressed = !is_release;
    else if (make_code == 0x5B) win_pressed = !is_release;
    
    if (!is_release && ctrl_pressed && make_code == 0x2E) {
        if (foreground_task_id > 1) { 
            keyboard_flush();
            kill_task(foreground_task_id);
            printf("^C\n"); 
        }
        return;
    }

    if (!is_release) {
        add_scancode_to_buffer(make_code);
        unsigned int c = scancode_to_char_layout(make_code);
        if (c != 0) add_to_buffer(c);
    }
}

unsigned int getch(void) {
    __asm__ volatile("sti");
    while (1) { if (!is_buffer_empty()) break; task_scheduler(); }
    __asm__ volatile("cli"); unsigned int c = get_from_buffer(); __asm__ volatile("sti"); return c;
}
uint8_t wait_scancode(void) {
    __asm__ volatile("sti");
    while (1) { if (!is_scancode_buffer_empty()) break; task_scheduler(); }
    __asm__ volatile("cli"); uint8_t code = get_scancode_from_buffer(); __asm__ volatile("sti"); return code;
}
void gets(char* buffer, int max_len) {
    int pos = 0;
    while (1) {
        unsigned int c = getch();
        if (c == '\n') { buffer[pos] = '\0'; printf("\n"); return; }
        else if (c == '\b') { if (pos > 0) { pos--; buffer[pos] = '\0'; printf("\b \b"); } }
        else if (c != 0) { if (pos < max_len - 1) { buffer[pos++] = (char)c; printf("%c", c); } }
    }
}