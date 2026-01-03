#ifndef LIB_H
#define LIB_H

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

void print_char(char c);
void print_char_colored(char c, int color);
void print_colored(char *str, int color);
void print(char *str);
void exit(void);
char getc(void);
void gets(char *buffer, int max_len);
void *malloc(int size);
void free(void *ptr);
void cls(void);
unsigned long random(void);
unsigned long randmm(unsigned long min, unsigned long max);
void print_number(int number);
void draw_simple_box(char **lines, char *title, uint8_t centered);
void set_cursor_position(unsigned int x, unsigned int y);

#endif