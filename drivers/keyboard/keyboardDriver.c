#include "drivers/keyboard/keyboardDriver.h"
#include "drivers/vga/vga.h"
#include "drivers/io/io.h"
#include "task/task.h"
#include "drivers/video/graphics.h"
#include "system_apps/console/console.h"
#include "global.h"
#include "drivers/timer/timer.h" 

#define BUFFER_SIZE 256

static uint8_t shift_pressed = 0;
static uint8_t caps_lock = 0;

static unsigned int key_buffer[BUFFER_SIZE];
static volatile int buffer_count = 0;

static uint8_t scancode_buffer[BUFFER_SIZE];
static volatile int scancode_count = 0;

uint8_t current_layout = 0;

static void add_to_buffer(unsigned int c) {
    if (buffer_count < BUFFER_SIZE) {
        key_buffer[buffer_count] = c;
        buffer_count++;
    }
}

static int is_buffer_empty(void) {
    return buffer_count == 0;
}

static unsigned int get_from_buffer(void) {
    if (buffer_count > 0) {
        unsigned int c = key_buffer[0];
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

static unsigned int get_letter(unsigned int lower, unsigned int upper, uint8_t shift, uint8_t caps) {
    if (shift) return upper;
    if (caps) return upper;
    return lower;
}

unsigned int scancode_to_char(uint8_t scancode) {
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

unsigned int scancode_to_char_ru(uint8_t scancode) {
    if (scancode & 0x80) return 0;
    
    switch(scancode) {
        case 0x10: return get_letter(0x0439, 0x0419, shift_pressed, caps_lock); 
        case 0x11: return get_letter(0x0446, 0x0426, shift_pressed, caps_lock); 
        case 0x12: return get_letter(0x0443, 0x0423, shift_pressed, caps_lock); 
        case 0x13: return get_letter(0x043A, 0x041A, shift_pressed, caps_lock); 
        case 0x14: return get_letter(0x0435, 0x0415, shift_pressed, caps_lock); 
        case 0x15: return get_letter(0x043D, 0x041D, shift_pressed, caps_lock); 
        case 0x16: return get_letter(0x0433, 0x0413, shift_pressed, caps_lock); 
        case 0x17: return get_letter(0x0448, 0x0428, shift_pressed, caps_lock); 
        case 0x18: return get_letter(0x0449, 0x0429, shift_pressed, caps_lock); 
        case 0x19: return get_letter(0x0437, 0x0417, shift_pressed, caps_lock); 
        case 0x1A: return get_letter(0x0445, 0x0425, shift_pressed, caps_lock); 
        case 0x1B: return get_letter(0x044A, 0x042A, shift_pressed, caps_lock); 
        case 0x1E: return get_letter(0x0444, 0x0424, shift_pressed, caps_lock); 
        case 0x1F: return get_letter(0x044B, 0x042B, shift_pressed, caps_lock); 
        case 0x20: return get_letter(0x0432, 0x0412, shift_pressed, caps_lock); 
        case 0x21: return get_letter(0x0430, 0x0410, shift_pressed, caps_lock); 
        case 0x22: return get_letter(0x043F, 0x041F, shift_pressed, caps_lock); 
        case 0x23: return get_letter(0x0440, 0x0420, shift_pressed, caps_lock); 
        case 0x24: return get_letter(0x043E, 0x041E, shift_pressed, caps_lock); 
        case 0x25: return get_letter(0x043B, 0x041B, shift_pressed, caps_lock); 
        case 0x26: return get_letter(0x0434, 0x0414, shift_pressed, caps_lock); 
        case 0x27: return get_letter(0x0436, 0x0416, shift_pressed, caps_lock); 
        case 0x28: return get_letter(0x044D, 0x042D, shift_pressed, caps_lock); 
        case 0x2C: return get_letter(0x044F, 0x042F, shift_pressed, caps_lock); 
        case 0x2D: return get_letter(0x0447, 0x0427, shift_pressed, caps_lock); 
        case 0x2E: return get_letter(0x0441, 0x0421, shift_pressed, caps_lock); 
        case 0x2F: return get_letter(0x043C, 0x041C, shift_pressed, caps_lock); 
        case 0x30: return get_letter(0x0438, 0x0418, shift_pressed, caps_lock); 
        case 0x31: return get_letter(0x0442, 0x0422, shift_pressed, caps_lock); 
        case 0x32: return get_letter(0x044C, 0x042C, shift_pressed, caps_lock); 
        case 0x33: return get_letter(0x0431, 0x0411, shift_pressed, caps_lock); 
        case 0x34: return get_letter(0x044E, 0x042E, shift_pressed, caps_lock); 
        case 0x35: return shift_pressed ? ',' : '.';
        case 0x29: return get_letter(0x0435, 0x0415, shift_pressed, caps_lock); 
        default: return scancode_to_char(scancode); 
    }
}

unsigned int scancode_to_char_layout(uint8_t scancode) {
    if (current_layout == 0) return scancode_to_char(scancode);
    return scancode_to_char_ru(scancode);
}

void keyboard_handler_c(void) {
    uint8_t scancode = inb(0x60);
    uint8_t is_release = (scancode & 0x80);
    uint8_t make_code = scancode & 0x7F;
    
    
    static unsigned long last_console_open = 0;
    static unsigned long last_kill_time = 0;
    static unsigned long last_focus_time = 0;
    static unsigned long last_ws_time = 0;

    if (make_code == 0x2A || make_code == 0x36) shift_pressed = !is_release;
    else if (make_code == 0x3A && !is_release) caps_lock = !caps_lock;

    static uint8_t alt_held = 0;
    static uint8_t ctrl_held = 0;
    if (make_code == 0x38) alt_held = !is_release;
    if (make_code == 0x1D) ctrl_held = !is_release;

    if (ctrl_held && alt_held && shift_pressed && !is_release && focused_window) {
        if (make_code == key_ws_left || make_code == key_resize_up) { wm_swap_window(-1); return; }
        if (make_code == key_ws_right || make_code == key_resize_down) { wm_swap_window(1); return; }
    }

    if (alt_held && shift_pressed && !ctrl_held && !is_release && focused_window) {
        if (make_code == key_resize_left) {
            focused_window->stretch_x -= 10;
            if (focused_window->stretch_x < 10) focused_window->stretch_x = 10;
            wm_refresh(); return;
        }
        if (make_code == key_resize_right) {
            focused_window->stretch_x += 10;
            wm_refresh(); return;
        }
        if (make_code == key_resize_up) {
            focused_window->stretch_y -= 10;
            if (focused_window->stretch_y < 10) focused_window->stretch_y = 10;
            wm_refresh(); return;
        }
        if (make_code == key_resize_down) {
            focused_window->stretch_y += 10;
            wm_refresh(); return;
        }
    }

    
    if (alt_held && !shift_pressed && !ctrl_held && !is_release) {
        if (get_ticks() - last_ws_time > 200) {
            if (make_code == key_ws_left) { wm_switch_workspace(-1); last_ws_time = get_ticks(); return; }
            if (make_code == key_ws_right) { wm_switch_workspace(1); last_ws_time = get_ticks(); return; }
        } else {
            if (make_code == key_ws_left || make_code == key_ws_right) return;
        }
    }

    if (shift_pressed && make_code == key_fullscreen && !is_release) { wm_toggle_fullscreen(); return; }
    if (alt_held && make_code == key_layout && !is_release) { current_layout = !current_layout; return; }
    
    
    if (alt_held && make_code == key_focus && !is_release) { 
        wm_switch_focus(); last_focus_time = get_ticks();
        return; 
    }
    
    
    if (alt_held && make_code == key_kill && !is_release){ 
        kill_focused_process(); keyboard_flush(); last_kill_time = get_ticks();
        return; 
    }

    
    if (alt_held && make_code == key_console && !is_release){ 
        create_process((void (*)(int, char**))console.main, 0, 0, "console", kernel_dir); 
        last_console_open = get_ticks();
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
    while (1) {
        if (!is_buffer_empty()) {
            if (focused_window == 0 || current_task->window == focused_window) break;
        }
        task_scheduler();
    }
    __asm__ volatile("cli");
    unsigned int c = get_from_buffer();
    __asm__ volatile("sti");
    return c;
}

uint8_t wait_scancode(void) {
    __asm__ volatile("sti");
    while (1) {
        if (!is_scancode_buffer_empty()) {
            if (focused_window == 0 || current_task->window == focused_window) break;
        }
        task_scheduler();
    }
    __asm__ volatile("cli");
    uint8_t code = get_scancode_from_buffer();
    __asm__ volatile("sti");
    return code;
}

void gets(char* buffer, int max_len) {
    int pos = 0;
    while (1) {
        unsigned int c = getch();
        if (c == '\n') {
            buffer[pos] = '\0';
            printf("\n");
            return;
        }
        else if (c == '\b') {
            if (pos > 0) {
                pos--;
                while (pos > 0 && (buffer[pos] & 0xC0) == 0x80) pos--;
                buffer[pos] = '\0';
                printf("\b \b");
            }
        }
        else if (c != 0) {
            char tmp[4];
            int bytes = 0;
            if (c < 0x80) { tmp[0] = c; bytes = 1; }
            else if (c < 0x800) { tmp[0] = 0xC0 | (c >> 6); tmp[1] = 0x80 | (c & 0x3F); bytes = 2; }
            else { tmp[0] = 0xE0 | (c >> 12); tmp[1] = 0x80 | ((c >> 6) & 0x3F); tmp[2] = 0x80 | (c & 0x3F); bytes = 3; }
            
            if (pos + bytes < max_len - 1) {
                for(int i=0; i<bytes; i++) buffer[pos++] = tmp[i];
                printf("%c", c);
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