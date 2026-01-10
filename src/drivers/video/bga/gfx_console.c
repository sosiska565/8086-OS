#include "drivers/video/bga/bga.h"

static int term_x = 0;
static int term_y = 0;
static uint32_t term_fg_color = 0x00FFFFFF;
static uint32_t term_bg_color = 0x00000000;

#define FONT_W 8
#define FONT_H 8

int term_cols;
int term_rows;

void init_gfx_console(void) {
    term_x = 0;
    term_y = 0;
    term_cols = screenW / FONT_W;
    term_rows = screenH / FONT_H;
}

void gfx_scroll(void) {
    
    uint32_t *vram = video_memory;
    
    uint32_t *dest = vram;
    uint32_t *src = vram + (screenW * FONT_H);
    
    int count = (screenH - FONT_H) * screenW;
    
    for (int i = 0; i < count; i++) {
        dest[i] = src[i];
    }
    
    int last_line_offset = (screenH - FONT_H) * screenW;
    for (int i = 0; i < FONT_H * screenW; i++) {
        vram[last_line_offset + i] = term_bg_color;
    }
}

void gfx_putc(char c) {
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
        bga_draw_char(term_x * FONT_W, term_y * FONT_H, c, term_fg_color, term_bg_color);
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