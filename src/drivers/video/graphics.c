#include "drivers/video/graphics.h"
#include "drivers/video/vesa.h"
#include "drivers/video/bga/font.h"
#include "drivers/vga/vga.h"

static int abs(int i) { return i < 0 ? -i : i; }
static void swap(int* a, int* b) { int t = *a; *a = *b; *b = t; }

Window *current_output_window = 0;

void set_current_output_window(Window *win) {
    current_output_window = win;
}

void draw_rect(int x, int y, int w, int h, uint32_t color) {
    for (int i = 0; i < w; i++) {
        put_pixel(x + i, y, color);
        put_pixel(x + i, y + h - 1, color);
    }
    for (int i = 0; i < h; i++) {
        put_pixel(x, y + i, color);
        put_pixel(x + w - 1, y + i, color);
    }
}

void draw_rect_filled(int x, int y, int w, int h, uint32_t color) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            put_pixel(x + j, y + i, color);
        }
    }
}

void draw_line(int x1, int y1, int x2, int y2, uint32_t color) {
    int dx = abs(x2 - x1);
    int sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1);
    int sy = y1 < y2 ? 1 : -1;
    int err = dx + dy;
    
    while (1) {
        put_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

void draw_circle(int x0, int y0, int radius, uint32_t color) {
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    while (y >= x) {
        put_pixel(x0 + x, y0 + y, color);
        put_pixel(x0 - x, y0 + y, color);
        put_pixel(x0 + x, y0 - y, color);
        put_pixel(x0 - x, y0 - y, color);
        put_pixel(x0 + y, y0 + x, color);
        put_pixel(x0 - y, y0 + x, color);
        put_pixel(x0 + y, y0 - x, color);
        put_pixel(x0 - y, y0 - x, color);
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}

void draw_circle_filled(int x0, int y0, int radius, uint32_t color) {
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    while (y >= x) {
        draw_line(x0 - x, y0 + y, x0 + x, y0 + y, color);
        draw_line(x0 - x, y0 - y, x0 + x, y0 - y, color);
        draw_line(x0 - y, y0 + x, x0 + y, y0 + x, color);
        draw_line(x0 - y, y0 - x, x0 + y, y0 - x, color);
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}

void draw_window(
    Window *win,
    int x, int y,
    int width, int height,
    uint32_t color_frame, uint32_t color_window,
    int centered
){
    if(centered == 1){
        x = (get_screen_width() - width) / 2;
        y = (get_screen_height() - height) / 2;
    }
    win->x = x;
    win->y = y;
    win->width = width;
    win->height = height;
    win->bg_color = color_window;
    win->cursor_x = 0;
    win->cursor_y = 0;
    win->text_color = 0xFFFFFF;

    draw_rect_filled(x - 3, y - 3, width + 6, height + 6, color_frame);
    draw_rect_filled(x, y, width, height, color_window);
}

void window_put_pixel(Window *win, int x, int y, uint32_t color){
    if(x < 0 || x >= win->width || y < 0 || y >= win->height) return;

    put_pixel(win->x + x, win->y + y, color);
}

void window_clear(Window *win, uint32_t color){
    for(int i = 0; i < win->height; i++){
        for(int j = 0; j < win->width; j++){
            window_put_pixel(win, j, i, color);
        }
    }
    win->cursor_x = 0;
    win->cursor_y = 0;
}

void window_draw_char(Window *win, int x, int y, char c, uint32_t color) {
    uint8_t *glyph = (uint8_t*)font8x8_basic[(int)((unsigned char)c)];

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            int bit = (glyph[row] >> col) & 1;
            
            if (bit) {
                window_put_pixel(win, x + col, y + row, color);
            }
        }
    }
}

void window_print(Window *win, int x, int y, char *str, uint32_t color) {
    int cursor_x = x;
    int cursor_y = y;
    
    while (*str) {
        window_draw_char(win, cursor_x, cursor_y, *str, color);
        cursor_x += 8;
        str++;
    }
}

void window_scroll(Window *win){
    window_clear(win, win->bg_color);
    win->cursor_x = 0;
    win->cursor_y = 0;
}

void window_putc(Window *win, char c) {
    if (win == 0) return;

    if (c == '\n') {
        win->cursor_x = 0;
        win->cursor_y += 8;
    } 
    else if (c == '\b') {
        if (win->cursor_x >= 8) {
            win->cursor_x -= 8;
            for(int y = 0; y < 8; y++) {
                for(int x = 0; x < 8; x++) {
                    window_put_pixel(win, win->cursor_x + x, win->cursor_y + y, win->bg_color);
                }
            }
        }
    }
    else {
        window_draw_char(win, win->cursor_x, win->cursor_y, c, win->text_color);
        win->cursor_x += 8;
    }

    if (win->cursor_x >= win->width - 8) {
        win->cursor_x = 0;
        win->cursor_y += 8;
    }

    if (win->cursor_y >= win->height - 8) {
        window_scroll(win);
    }
}