#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

typedef struct {
    int x, y;
    int width, height;
    uint32_t bg_color;

    int cursor_x, cursor_y;
    uint32_t text_color;
} Window;

void draw_rect(int x, int y, int w, int h, uint32_t color);
void draw_rect_filled(int x, int y, int w, int h, uint32_t color);
void draw_line(int x1, int y1, int x2, int y2, uint32_t color);
void draw_circle(int x0, int y0, int radius, uint32_t color);
void draw_circle_filled(int x0, int y0, int radius, uint32_t color);
void draw_window(Window *win, int x, int y, int width, int height, uint32_t color_frame, uint32_t color_window, int centered);
void window_draw_char(Window *win, int x, int y, char c, uint32_t color);
void window_print(Window *win, int x, int y, char *str, uint32_t color);
void window_clear(Window *win, uint32_t color);
void set_current_output_window(Window *win);
void window_putc(Window *win, char c);

#endif