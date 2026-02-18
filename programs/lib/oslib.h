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

#define VGA_COLOR(fg, bg) ((bg << 4) | (fg & 0x0F))

typedef struct {
    int x;
    int y;
    int width;
    int height;
    uint32_t color;
} Rect;

typedef struct Window{
    int id;
    int x, y;
    int width, height;
    uint32_t bg_color;
    uint32_t border_color;

    int cursor_x, cursor_y;
    uint32_t text_color;

    struct Window *next;

    char *char_buffer;
    uint32_t *color_buffer;
    int rows, cols;    
} Window;

typedef struct text_struct{
    int x;
    int y;
    char *str;
    uint32_t color;
} text_struct;

typedef struct process_struct{
    void (*foo)(int, char**);
    int argc;
    char **argv;
    char *name;
} process_struct;

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
void set_cursor_position(unsigned int x, unsigned int y);
void set_background_color(vga_color_t color);
void set_text_color(vga_color_t color);
void get_cursor_xy(unsigned int *x, unsigned int *y);
vga_color_t get_current_color(void);
void set_current_color(vga_color_t color);
int read_file(char *file_name, uint8_t *file_buffer);
void printhex(unsigned int num);
int get_file_size(char *file_name);
uint8_t get_scanecode(void);
int write_file(char *filename, uint8_t *buffer, uint32_t size);
char scancode_to_ascii(uint8_t scancode);
void memcpy(void* dest, void* src, int size);
int getScreenWidth(void);
int getScreenHeight(void);
void draw_rect_filled(Rect *rect);
int get_screen_width(void);
int get_screen_height(void);
void draw_window(Window *win);
void set_current_active_window(Window *win);
Window *create_window(uint32_t bg_color);
void close_window(Window *win);
void printf(const char* format, ...);
int strcmp(const char *c1, const char *c2);
void print_window(Window *win, text_struct* ts);
void sleep(unsigned long ms);
int fork(process_struct *p);
void kill(int pid);

#endif