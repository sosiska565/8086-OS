/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/drivers/vga/vga.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include "drivers/vga/vga.h"
#include "drivers/io/io.h"
#include "drivers/video/bga/gfx_console.h"
#include <stddef.h>
#include "global.h"
#include "drivers/video/vesa.h"
#include "task/task.h"
#include "utils/utils.h"

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
    gfx_set_cursor(0, 0);
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
    else if(c == '\t'){
        move_cursor_next_line();
        move_cursor_next_line();
        move_cursor_next_line();
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

int strcmp(const char *c1, const char *c2) {
    for(int i = 0; ; i++) {
        if(c1[i] != c2[i]) {
            return c1[i] - c2[i];
        }
        if(c1[i] == '\0') {
            return 0;
        }
    }
}

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

void strcpy(char *dst, const char *src){
    int i = 0;
    while (src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

int strlen(const char *str){
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


char toupper_char(unsigned int c) {
    if (c >= 'a' && c <= 'z') return c - 32;
    return c;
}

void set_current_color(vga_color_t color){
    current_color = color;
} 

void _putchar(unsigned int c) {
    gfx_putc(c); 
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

void print_utf8_string(const char* s) {
    while (*s) {
        unsigned int code;
        s = utf8_to_unicode(s, &code);
        _putchar(code);
    }
}

void _set_console_color(unsigned int color) {
    gfx_set_color((color <= 15) ? vga_to_rgb[color] : color);
}

extern void gfx_start_batch(void);
extern void gfx_end_batch(void);

void vprintf(const char* format, va_list args) {
    gfx_start_batch();
    while (*format != '\0') {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 'c': _putchar((unsigned int)va_arg(args, int)); break;
                case 's': {
                    char* s = va_arg(args, char*);
                    print_utf8_string(s);
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
            unsigned int code = 0;
            format = utf8_to_unicode(format, &code);
            _putchar(code);
            continue;
        }
        format++;
    }

    _set_console_color(DEF_COLOR_GFX);
    gfx_end_batch();
}

void printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    vprintf(format, args);
    
    va_end(args);
}

void reverse(char str[], int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        unsigned int temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

void itoa(unsigned int n, char* buffer, int base) {
    int i = 0;
    if (n == 0) { buffer[i++] = '0'; buffer[i] = 0; return; }
    
    while (n != 0) {
        unsigned int rem = n % base;
        buffer[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        n = n / base;
    }
    buffer[i] = 0;
    
    for(int j = 0; j < i / 2; j++) {
        char tmp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = tmp;
    }
}

unsigned int tolower(unsigned int c){
    if(c >= 'A' && c <= 'Z'){
        return c + ('a' - 'A');
    }
    return c;
}

long atoi(const char *str, int base){
    long result = 0;
    int sign = 1;
    int i = 0;

    while(is_space(str[i])) i++;

    if(str[i] == '-'){ sign = -1; i++; } 
    else if(str[i] == '+'){ i++; }

    
    if (base == 16 && str[i] == '0' && (str[i+1] == 'x' || str[i+1] == 'X')) {
        i += 2;
    }

    while(str[i] != '\0'){
        int digit = -1;
        unsigned int c = tolower(str[i]);

        if(c >= '0' && c <= '9') digit = c - '0';
        else if(c >= 'a' && c <= 'z') digit = c - 'a' + 10;

        if(digit == -1 || digit >= base) break;

        result = result * base + digit;
        i++;
    }
    return result * sign;
}

void strcat(char *dest, const char* str){
    while(*dest != '\0'){
        dest++;
    }

    while(*str != '\0'){
        *dest = *str;
        dest++;
        str++;
    }

    *dest = '\0';
}

int strncmp(const char *s1, const char *s2, int n) {
    while (n > 0 && *s1 && (*s1 == *s2)) {
        ++s1;
        ++s2;
        --n;
    }
    if (n == 0) {
        return 0;
    }
    return (*(unsigned char *)s1 - *(unsigned char *)s2);
}
