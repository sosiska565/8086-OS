#ifndef VGA_H
#define VGA_H

#include <stdint.h>
#include <stddef.h>

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

//utils

void scroll_screen(void);
int strcmp(char *c1, char *c2);
char **parse_str(char *str, char parse_char);

//

void print_header(int header_bg_color, int header_text_color, char *title);
void print_footer(int footer_bg_color, int footer_text_color, char *text);
void print_info(char *status, char *info, unsigned short color_status, unsigned short color_info);
int strtn(char *str);
void strcpy(char *s1, char *s2);
void *memset(void *ptr, int value, size_t num);
char* toupper(char *str);
char toupper_char(char c);
void set_current_color(vga_color_t color);
int strlen(char *str);

//псевдо графика
void draw_text_box_ex(char* lines[], char* title, 
                      uint8_t padding_top, uint8_t padding_bottom,
                      uint8_t padding_left, uint8_t padding_right,
                      uint8_t border_color, uint8_t text_color, uint8_t title_color, uint8_t centered);
void draw_text_box(char* lines[], char* title, uint8_t padding, 
                   uint8_t border_color, uint8_t text_color, uint8_t title_color, uint8_t centered);
void draw_simple_box(char* lines[], char* title, uint8_t centered);

#endif