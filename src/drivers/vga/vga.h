#ifndef VGA_H
#define VGA_H

#include <stdint.h>

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
} vga_color_t;

#define VGA_COLOR(fg, bg) ((bg << 4) | (fg & 0x0F))

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

//utils

void scroll_screen(void);
int strcmp(char *c1, char *c2);
char **parse_str(char *str);

#endif