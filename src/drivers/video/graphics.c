#include "drivers/video/graphics.h"
#include "drivers/video/vesa.h"

static int abs(int i) { return i < 0 ? -i : i; }
static void swap(int* a, int* b) { int t = *a; *a = *b; *b = t; }

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