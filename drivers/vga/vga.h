#ifndef VGA_H
#define VGA_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

typedef enum {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_YELLOW = 14,
    VGA_COLOR_WHITE = 15,
    
    VGA32_COLOR_BLACK = 0x00000000,
    VGA32_COLOR_BLUE = 0x000000FF,
    VGA32_COLOR_GREEN = 0x0000FF00,
    VGA32_COLOR_CYAN = 0x0000FFFF,
    VGA32_COLOR_RED = 0x00FF0000,
    VGA32_COLOR_MAGENTA = 0x00FF00FF,
    VGA32_COLOR_BROWN = 0x00640000,
    VGA32_COLOR_LIGHT_GREY = 0x00D3D3D3,
    VGA32_COLOR_DARK_GREY = 0x00595959,
    VGA32_COLOR_LIGHT_BLUE = 0x00ADD8E6,
    VGA32_COLOR_LIGHT_GREEN = 0x0090EE90,
    VGA32_COLOR_LIGHT_CYAN = 0x00E0FFFF,
    VGA32_COLOR_LIGHT_RED = 0x00FFA07A,
    VGA32_COLOR_LIGHT_MAGENTA = 0x00FFB6C1,
    VGA32_COLOR_YELLOW = 0x00FFFF00,
    VGA32_COLOR_WHITE = 0x00FFFFFF,
} vga_color_t;

static const uint32_t vga_to_rgb[] = {
    0x000000,
    0x0000AA,
    0x00AA00,
    0x00AAAA,
    0xAA0000, 
    0xAA00AA,
    0xAA5500,
    0xAAAAAA,
    0x555555,
    0x5555FF,
    0x55FF55,
    0x55FFFF,
    0xFF5555,
    0xFF55FF,
    0xFFFF55,
    0xFFFFFF
};

#define VGA_COLOR(fg, bg) ((bg << 4) | (fg & 0x0F))

uint16_t vga_get_entry(int x, int y);
void vga_set_entry(int x, int y, uint16_t entry);

void clear_screen(void);
void clear_screen_colored(uint8_t color);

void print(const char* str);
void print_colored(const char* str, uint8_t color);

void printnumber(int num);
void printnumber_colored(int num, uint8_t color);

void printhex(unsigned int num);
void printhex_colored(unsigned int num, uint8_t color);

void print_char(char c);
void print_char_colored(char c, uint8_t color);

void printn(char *c);
void printn_void(void);
void printn_colored(char *c, uint8_t color);

void set_text_color(vga_color_t fg);      
void set_background_color(vga_color_t bg);
void set_color(vga_color_t fg, vga_color_t bg);
uint8_t get_current_color(void);

void update_cursor(void);
void disable_cursor(void);
void enable_cursor(void);
void enable_cursor_block(void);
unsigned int get_cursor_position(void);
void set_cursor_position(unsigned int x, unsigned int y);
void move_cursor_next_line(void);
void get_cursor_xy(unsigned int *x, unsigned int *y);


void scroll_screen(void);
int strcmp(const char *c1, const char *c2); 
char **parse_str(char *str, char parse_char);

void print_header(int header_bg_color, int header_text_color, char *title);
void print_footer(int footer_bg_color, int footer_text_color, char *text);
void print_info(char *status, char *info, unsigned short color_status, unsigned short color_info);
int strtn(char *str);
void strcpy(char *s1, const char *s2);
void *memset(void *ptr, int value, size_t num);
char* toupper(char *str);
char toupper_char(unsigned int c);
void set_current_color(vga_color_t color);
int strlen(const char *str);
void printf(const char* format, ...);
void vprintf(const char* format, va_list args);
void itoa(unsigned int n, char* buffer, int base);
void strcat(char *dest, const char* str);
long atoi(const char *str, int base);
int strncmp(const char *s1, const char *s2, int n);

#endif