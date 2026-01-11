#include "drivers/vga/vga.h"
#include "drivers/io/io.h"
#include "drivers/video/bga/gfx_console.h"
#include <stddef.h>
#include "global.h"
#include "drivers/video/vesa.h"

#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE (LINES * COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT)

#define VGA_CTRL_REGISTER 0x3D4
#define VGA_DATA_REGISTER 0x3D5

#define CURSOR_LOCATION_HIGH 0x0E
#define CURSOR_LOCATION_LOW  0x0F
#define CURSOR_START_REG     0x0A
#define CURSOR_END_REG       0x0B

#define DEF_COLOR_TEXT 0x07
#define DEF_COLOR_GFX  0x00FFFFFF

unsigned int current_loc = 0;
char *vidptr = (char*) 0xb8000;
unsigned int lines = 0;
static uint8_t current_color = 0x07;
extern uint32_t *video_memory;

uint16_t vga_get_entry(int x, int y){
    unsigned int index = y * COLUMNS_IN_LINE + x;

    uint16_t result = vidptr[index * 2] | (vidptr[index * 2 + 1] << 8);
    return result;
}

void vga_set_entry(int x, int y, uint16_t entry){
    unsigned int index = y * COLUMNS_IN_LINE + x;
    vidptr[index * 2] = entry & 0xFF;
    vidptr[index * 2 + 1] = (entry >> 8) & 0xFF;
}

void set_text_color(vga_color_t fg) {
    
    current_color = (current_color & 0xF0) | (fg & 0x0F);
}

void set_background_color(vga_color_t bg) {
    if (bg <= 15) {
        gfx_set_bg_color(vga_to_rgb[bg]);
    } 
    else {
        gfx_set_bg_color(bg);
    }
}

void set_color(vga_color_t fg, vga_color_t bg) {
    current_color = ((bg & 0x0F) << 4) | (fg & 0x0F);
}

uint8_t get_current_color(void) {
    return current_color;
}

void update_cursor(void) {
    outb(VGA_CTRL_REGISTER, CURSOR_LOCATION_HIGH);
    outb(VGA_DATA_REGISTER, (current_loc >> 8) & 0xFF);
    
    outb(VGA_CTRL_REGISTER, CURSOR_LOCATION_LOW);
    outb(VGA_DATA_REGISTER, current_loc & 0xFF);
}

void disable_cursor(void) {
    outb(VGA_CTRL_REGISTER, CURSOR_START_REG);
    outb(VGA_DATA_REGISTER, 0x20);
}

void enable_cursor(void) {
    outb(VGA_CTRL_REGISTER, CURSOR_START_REG);
    outb(VGA_DATA_REGISTER, 0x0E);
    
    outb(VGA_CTRL_REGISTER, CURSOR_END_REG);
    outb(VGA_DATA_REGISTER, 0x0F);
}

void enable_cursor_block(void) {
    outb(VGA_CTRL_REGISTER, CURSOR_START_REG);
    outb(VGA_DATA_REGISTER, 0x00);
    
    outb(VGA_CTRL_REGISTER, CURSOR_END_REG);
    outb(VGA_DATA_REGISTER, 0x0F);
}

unsigned int get_cursor_position(void) {
    unsigned int pos = 0;
    
    outb(VGA_CTRL_REGISTER, CURSOR_LOCATION_HIGH);
    pos = inb(VGA_DATA_REGISTER) << 8;
    
    outb(VGA_CTRL_REGISTER, CURSOR_LOCATION_LOW);
    pos |= inb(VGA_DATA_REGISTER);
    
    return pos;
}

void set_cursor_position(unsigned int x, unsigned int y) {
    gfx_set_cursor((int)x, (int)y);
}

void move_cursor_next_line(void) {
    current_loc += COLUMNS_IN_LINE - (current_loc % COLUMNS_IN_LINE);
    
    if (current_loc >= COLUMNS_IN_LINE * LINES) {
        scroll_screen();
        current_loc = (LINES - 1) * COLUMNS_IN_LINE;
    }
    
    update_cursor();
}

void get_cursor_xy(unsigned int *x, unsigned int *y) {
    int ix, iy;
    gfx_get_cursor(&ix, &iy);
    *x = (unsigned int)ix;
    *y = (unsigned int)iy;
}

void scroll_screen(void) {
    for (unsigned int i = 0; i < (LINES - 1) * COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT; i++) {
        vidptr[i] = vidptr[i + COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT];
    }
    
    unsigned int last_line_start = (LINES - 1) * COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT;
    for (unsigned int i = 0; i < COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT; i += 2) {
        vidptr[last_line_start + i] = ' ';
        vidptr[last_line_start + i + 1] = current_color;
    }
}

void clear_screen(void) {
    clear_screen_vesa(0x00000000);
    set_cursor_position(0, 0);
}

void clear_screen_colored(uint8_t color) {
    unsigned int i = 0;
    while(i < SCREENSIZE) {
        vidptr[i++] = ' ';
        vidptr[i++] = color;
    }
    current_loc = 0;
    lines = 0;
    update_cursor();
}

void print_char(char c) {
    print_char_colored(c, current_color);
}

void print_char_colored(char c, uint8_t color) {
    if(c == '\n') {
        move_cursor_next_line();
    } 
    else if(c == '\b') {
        if(current_loc > 0) {
            current_loc--;
            vidptr[current_loc * 2] = ' ';
            vidptr[current_loc * 2 + 1] = color;
            update_cursor();
        }
    } 
    else {
        vidptr[current_loc * 2] = c;
        vidptr[current_loc * 2 + 1] = color;
        current_loc++;
        
        if (current_loc >= COLUMNS_IN_LINE * LINES) {
            scroll_screen();
            current_loc = (LINES - 1) * COLUMNS_IN_LINE;
        }
        
        update_cursor();
    }
}

void print(const char* str) {
    print_colored(str, current_color);
}

void print_colored(const char* str, uint8_t color) {
    unsigned int i = 0;
    while(str[i] != '\0') {
        print_char_colored(str[i], color); 
        i++;
    }
}

void printn(char *c) {
    printf("\n");
    printf(c);
}

void printn_void(void) {
    printf("\n");
}

void printn_colored(char *c, uint8_t color) {
    print_char_colored('\n', color);
    print_colored(c, color);
}

void printnumber(int num) {
    printnumber_colored(num, current_color);
}

void printnumber_colored(int num, uint8_t color) {
    char buffer[32];
    int i = 0, isNegative = 0;

    if(num < 0) {
        isNegative = 1;
        num = -num;
    }

    do {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    } while(num > 0);

    if(isNegative) {
        buffer[i++] = '-';
    }

    for(int j = i - 1; j >= 0; j--) {
        vidptr[current_loc * 2] = buffer[j];
        vidptr[current_loc * 2 + 1] = color;
        current_loc++;
        
        if(current_loc % COLUMNS_IN_LINE == 0) {
            lines++;
            
            if(current_loc >= COLUMNS_IN_LINE * LINES) {
                scroll_screen();
                current_loc = (LINES - 1) * COLUMNS_IN_LINE;
            }
        }
    }
    
    update_cursor();
}

void printhex(unsigned int num) {
    printhex_colored(num, current_color);
}

void printhex_colored(unsigned int num, uint8_t color) {
    char buffer[32];
    int i = 0;

    print_colored("0x", color);

    if(num == 0) {
        buffer[i++] = '0';
    } else {
        while(num > 0) {
            uint32_t remainder = num % 16;

            if(remainder < 10) {
                buffer[i++] = remainder + '0';
            } else {
                buffer[i++] = remainder - 10 + 'A';
            }

            num /= 16;
        }
    }

    for(int j = i - 1; j >= 0; j--) {
        vidptr[current_loc * 2] = buffer[j];
        vidptr[current_loc * 2 + 1] = color;
        current_loc++;
        
        if(current_loc % COLUMNS_IN_LINE == 0) {
            lines++;
            
            if(current_loc >= COLUMNS_IN_LINE * LINES) {
                scroll_screen();
                current_loc = (LINES - 1) * COLUMNS_IN_LINE;
            }
        }
    }
    
    update_cursor();
}

//utils

char** parse_str(char* str, char parse_char) {
    static char* tokens[256]; 
    static char buffer[256]; 
    
    if (!str) {
        tokens[0] = 0;
        return tokens;
    }
    
    int token_count = 0;
    char* ptr = buffer;
    
    while (*str && token_count < 255) {
        while (*str == parse_char) {
            str++;
        }
        
        if (!*str) break;
        
        tokens[token_count] = ptr;
        
        while (*str && *str != parse_char) {
            *ptr++ = *str++;
        }
        
        *ptr++ = '\0';
        token_count++;
    }
    
    tokens[token_count] = 0;
    
    return tokens;
}

int strcmp(char *c1, char *c2) {
    for(int i = 0; ; i++) {
        if(c1[i] != c2[i]) {
            return c1[i] - c2[i];
        }
        if(c1[i] == '\0') {
            return 0;
        }
    }
}

//

void print_header(int header_bg_color, int header_text_color, char *title){
    int len = 0;
    while(title[len] != '\0') len++;

    int padding = (80 - len) / 2;

    set_background_color(header_bg_color);
    set_text_color(header_text_color);

    for(int i = 0; i < padding; i++){
        printf(" ");
    }
    printf(title);
    for(int i = 0; i <= padding; i++){
        printf(" ");
    }

    set_background_color(VGA_COLOR_BLACK);
    set_text_color(VGA_COLOR_LIGHT_GREY);
}

void print_footer(int footer_bg_color, int footer_text_color, char *text) {
    int len = 0;
    int line_breaks = 0;
    while(text[len] != '\0') len++;
    
    if (len > 80) {
        line_breaks = (len - 1) / 80;
    }

    set_cursor_position(0, 24 - line_breaks);
    
    set_background_color(footer_bg_color);
    set_text_color(footer_text_color);
    
    int total_parts = (len + 79) / 80;
    char *parts[10];
    
    int part_index = 0;
    int chars_processed = 0;
    
    while (chars_processed < len) {
        int part_length = len - chars_processed;
        if (part_length > 80) part_length = 80;
        
        char *part = text + chars_processed;
        parts[part_index] = part;
        
        chars_processed += part_length;
        part_index++;
    }
    
    int last_part_length = len % 80;
    if (last_part_length == 0 && len > 0) {
        last_part_length = 80;
    }
    
    printf(text);
    
    for(int i = 0; i < 79 - 3; i++) {
        printf(" ");
    }
    
    set_background_color(VGA_COLOR_BLACK);
    set_text_color(VGA_COLOR_LIGHT_GREY);
}

void print_info(char *status, char *info, unsigned short color_status, unsigned short color_info){
    printf("[");
    print_colored(status, color_status);
    printf("] ");
    print_colored(info, color_info);
}

int strtn(char *str){
    int n = 0;
    while(*str >= '0' && *str <= '9'){
        n = n * 10 + (*str - '0');
        str++;
    }

    return n;
}

void strcpy(char *dst, char *src){
    int i = 0;
    while (src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

int strlen(char *str){
    int i = 0;
    for(; str[i] != '\0'; i++);
    return i;
}

void *memset(void *ptr, int value, size_t num) {
    unsigned char *p = ptr;
    unsigned char byte_value = (unsigned char)value;
    
    for(size_t i = 0; i < num; i++) {
        p[i] = byte_value;
    }
    
    return ptr;
}

char* toupper(char *str){
    static char buffer[256];
    int i;
    
    for(i = 0; str[i] != '\0' && i < 255; i++){
        if (str[i] >= 'a' && str[i] <= 'z') 
            buffer[i] = str[i] - 32;
        else
            buffer[i] = str[i];
    }
    buffer[i] = '\0';
    
    return buffer;
}


char toupper_char(char c) {
    if (c >= 'a' && c <= 'z') return c - 32;
    return c;
}

void set_current_color(vga_color_t color){
    current_color = color;
} 

void _putchar(char c) {
    if (video_memory != 0) {
        gfx_putc(c);
    }
}

void _puts(const char* str) {
    while(*str) {
        _putchar(*str++);
    }
}

void _print_number(int n, int base, int is_signed) {
    char buffer[64];
    int i = 0;
    int is_neg = 0;

    if (is_signed && n < 0 && base == 10) {
        is_neg = 1;
        n = -n;
    }

    if (n == 0) {
        _putchar('0');
        return;
    }

    unsigned int un = (unsigned int)n;
    
    while (un > 0) {
        int remainder = un % base;
        if (remainder >= 10) {
            buffer[i++] = (remainder - 10) + 'A';
        } else {
            buffer[i++] = remainder + '0';
        }
        un /= base;
    }

    if (is_neg) {
        _putchar('-');
    }

    while (--i >= 0) {
        _putchar(buffer[i]);
    }
}

void _set_console_color(unsigned int color) {
    if (color <= 15) {
        gfx_set_color(vga_to_rgb[color]);
    } 
    else {
        gfx_set_color(color);
    }
}

void vprintf(const char* format, va_list args) {
    while (*format != '\0') {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 'c': _putchar((char)va_arg(args, int)); break;
                case 's': {
                    char* s = va_arg(args, char*);
                    _puts(s ? s : "(null)"); 
                    break;
                }
                case 'd': 
                case 'i': _print_number(va_arg(args, int), 10, 1); break;
                case 'u': _print_number(va_arg(args, unsigned int), 10, 0); break;
                case 'x': 
                case 'X': 
                    _puts("0x"); 
                    _print_number(va_arg(args, unsigned int), 16, 0); 
                    break;
                case 'C': _set_console_color(va_arg(args, unsigned int)); break;
                case '%': _putchar('%'); break;
                default:  _putchar('%'); _putchar(*format); break;
            }
        } else {
            _putchar(*format);
        }
        format++;
    }

    _set_console_color(DEF_COLOR_GFX);
}

void printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    vprintf(format, args);
    
    va_end(args);
}

//псевдо графика
void draw_text_box_ex(char* lines[], char* title, 
                      uint8_t padding_top, uint8_t padding_bottom,
                      uint8_t padding_left, uint8_t padding_right,
                      uint8_t border_color, uint8_t text_color, uint8_t title_color,
                      uint8_t centered) {
    
    uint16_t max_width = 0;
    uint16_t line_count = 0;
    
    for (uint16_t i = 0; lines[i] != NULL; i++) {
        uint16_t len = 0;
        char* ptr = lines[i];
        while (*ptr != '\0') { len++; ptr++; }
        if (len > max_width) max_width = len;
        line_count++;
    }
    
    uint16_t title_len = 0;
    if (title != NULL) {
        char* ptr = title;
        while (*ptr != '\0') { title_len++; ptr++; }
        uint16_t needed_width = title_len + 4;
        if (needed_width > max_width) max_width = needed_width;
    }
    
    uint16_t inner_width = max_width + padding_left + padding_right;
    uint16_t box_width = inner_width + 2;
    
    uint8_t old_color = current_color;

    unsigned int start_x = 0;
    unsigned int start_y = 0;
    unsigned int temp_x;

    get_cursor_xy(&temp_x, &start_y);
    
    if (centered) {
        if (box_width < 80) {
            start_x = (80 - box_width) / 2;
        } else {
            start_x = 0;
        }
    } else {
        start_x = temp_x;
    }

    uint16_t current_row_offset = 0;
    set_cursor_position(start_x, start_y + current_row_offset);
    current_color = border_color;
    print_char(201);
    
    if (title != NULL) {
        uint8_t left_parts = 1;
        for (uint8_t i = 0; i < left_parts; i++) print_char(205);
        
        current_color = title_color;
        print_char(' ');
        printf(title);
        print_char(' ');
        current_color = border_color;
        
        uint16_t used = 2 + left_parts + title_len + 2;
        uint16_t right_parts = (box_width > used) ? (box_width - used) : 0;
        for (uint16_t i = 0; i < right_parts; i++) print_char(205);
    } else {
        for (uint16_t i = 0; i < box_width - 2; i++) print_char(205);
    }
    print_char(187);
    current_row_offset++;

    for (uint8_t row = 0; row < padding_top; row++) {
        set_cursor_position(start_x, start_y + current_row_offset);
        print_char(186);
        for (uint16_t i = 0; i < box_width - 2; i++) print_char(' ');
        print_char(186);
        current_row_offset++;
    }
    
    current_color = text_color;
    for (uint16_t line_idx = 0; line_idx < line_count; line_idx++) {
        set_cursor_position(start_x, start_y + current_row_offset);
        
        current_color = border_color;
        print_char(186);
        current_color = text_color;
        
        char* line = lines[line_idx];
        uint16_t line_len = 0;
        char* ptr = line;
        while (*ptr != '\0') { line_len++; ptr++; }
        
        if (centered) {
            uint16_t total_spaces = max_width - line_len;
            uint16_t spaces_left = (total_spaces / 2) + padding_left;
            uint16_t spaces_right = (total_spaces - (total_spaces / 2)) + padding_right;
            
            for (uint16_t i = 0; i < spaces_left; i++) print_char(' ');
            printf(line);
            for (uint16_t i = 0; i < spaces_right; i++) print_char(' ');
        } else {
            for (uint8_t i = 0; i < padding_left; i++) print_char(' ');
            printf(line);
            uint16_t spaces_needed = max_width - line_len + padding_right;
            for (uint16_t i = 0; i < spaces_needed; i++) print_char(' ');
        }
        
        current_color = border_color;
        print_char(186);
        current_row_offset++;
    }
    
    current_color = border_color;
    for (uint8_t row = 0; row < padding_bottom; row++) {
        set_cursor_position(start_x, start_y + current_row_offset);
        print_char(186);
        for (uint16_t i = 0; i < box_width - 2; i++) print_char(' ');
        print_char(186);
        current_row_offset++;
    }
    
    set_cursor_position(start_x, start_y + current_row_offset);
    print_char(200);
    for (uint16_t i = 0; i < box_width - 2; i++) print_char(205);
    print_char(188);
    printn_void();
    
    current_color = old_color;
}

void draw_text_box(char* lines[], char* title, uint8_t padding, 
                   uint8_t border_color, uint8_t text_color, uint8_t title_color,
                   uint8_t centered) {
    draw_text_box_ex(lines, title, padding, padding, padding, padding, 
                    border_color, text_color, title_color, centered);
}

void draw_simple_box(char* lines[], char* title, uint8_t centered) {
    draw_text_box(lines, title, 1, VGA_COLOR_WHITE, VGA_COLOR_LIGHT_GREY, 
                  VGA_COLOR_YELLOW, centered);
}