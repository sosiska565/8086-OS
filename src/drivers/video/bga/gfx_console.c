#include "drivers/video/vesa.h"

static int term_x = 0;
static int term_y = 0;
static uint32_t term_fg_color = 0x00FFFFFF;
static uint32_t term_bg_color = 0x00000000;

#define FONT_W 8
#define FONT_H 8

int term_cols;
int term_rows;

extern int screen_width;
extern int screen_height;
extern uint32_t *video_memory;

void init_gfx_console(void) {
    term_x = 0;
    term_y = 0;
    term_cols = screen_width / FONT_W;
    term_rows = screen_height / FONT_H;
}

void gfx_scroll(void) {
    
    uint32_t *vram = video_memory;
    
    uint32_t *dest = vram;
    uint32_t *src = vram + (screen_width * FONT_H);
    
    int count = (screen_height - FONT_H) * screen_width;
    
    for (int i = 0; i < count; i++) {
        dest[i] = src[i];
    }
    
    int last_line_offset = (screen_height - FONT_H) * screen_width;
    for (int i = 0; i < FONT_H * screen_width; i++) {
        vram[last_line_offset + i] = term_bg_color;
    }
}

void gfx_putc(unsigned int c) {
    if (c == '\n') {
        term_x = 0;
        term_y++;
    } 
    else if (c == '\b') {
        if (term_x > 0) term_x--;
        for(int y=0; y<FONT_H; y++) {
            for(int x=0; x<FONT_W; x++) {
                put_pixel(term_x * FONT_W + x, term_y * FONT_H + y, term_bg_color);
            }
        }
    } 
    else if (c >= 32) {
        vesa_draw_char(term_x * FONT_W, term_y * FONT_H, c, term_fg_color, term_bg_color);
        term_x++;
    }

    if (term_x >= term_cols) {
        term_x = 0;
        term_y++;
    }

    if (term_y >= term_rows) {
        gfx_scroll();
        term_y = term_rows - 1;
    }
}

void gfx_print(char *str) {
    while (*str) {
        gfx_putc(*str++);
    }
}

void gfx_set_color(uint32_t fg) {
    term_fg_color = fg;
}

void gfx_set_cursor(int x, int y) {
    if (x < 0) x = 0;
    if (x >= term_cols) x = term_cols - 1;
    
    if (y < 0) y = 0;
    if (y >= term_rows) y = term_rows - 1;

    term_x = x;
    term_y = y;
}

void gfx_get_cursor(int *x, int *y) {
    *x = term_x;
    *y = term_y;
}

void gfx_set_bg_color(uint32_t bg) {
    term_bg_color = bg;
}