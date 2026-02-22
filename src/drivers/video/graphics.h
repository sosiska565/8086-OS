#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

#define GAP 15
#define SCREEN_MARGIN 10

typedef struct Window{
    int id;
    int x, y;
    int width, height;
    uint32_t bg_color;
    uint32_t border_color;

    int cursor_x, cursor_y;
    uint32_t text_color;

    struct Window *next;

    unsigned int *char_buffer;
    uint32_t *color_buffer;
    int rows, cols;    
} Window;

typedef struct text_struct{
    int x;
    int y;
    char *str;
    uint32_t color;
} text_struct;

extern int window_count;
extern Window* focused_window;
extern Window* head;

void draw_rect(int x, int y, int w, int h, uint32_t color);
void draw_rect_filled(int x, int y, int w, int h, uint32_t color);
void draw_line(int x1, int y1, int x2, int y2, uint32_t color);
void draw_circle(int x0, int y0, int radius, uint32_t color);
void draw_circle_filled(int x0, int y0, int radius, uint32_t color);
void draw_window(Window *win, int x, int y, int width, int height, uint32_t color_frame, uint32_t color_window, int centered);
void window_draw_char(Window *win, int x, int y, unsigned int c, uint32_t color);
void window_print(Window *win, int x, int y, char *str, uint32_t color);
void window_clear(Window *win, uint32_t color);
void set_current_output_window(Window *win);
void wm_set_focused_window(Window *win);
void window_putc(Window *win, unsigned int c);

//window manager

void wm_close_window(Window *win);
Window* wm_create_window(uint32_t bg_color);
void wm_refresh();
void wm_switch_focus();
void wm_init();
void wm_render_window(Window *win);
void window_redraw_content(Window *win);

#endif