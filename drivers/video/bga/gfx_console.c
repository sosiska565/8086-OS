#include "drivers/video/bga/gfx_console.h"
#include "drivers/video/vesa.h"
#include "mm/memory.h"

static int term_x = 0;
static int term_y = 0;
static uint32_t term_fg_color = 0x00FFFFFF;
static uint32_t term_bg_color = 0x00000000;

#define FONT_W 8
#define FONT_H 8

int term_cols = 0;
int term_rows = 0;

extern int screen_width;
extern int screen_height;
extern int screen_pitch;
extern uint32_t *video_memory;
extern uint32_t *back_buffer;

void init_gfx_console(void) {
    term_x = 0;
    term_y = 0;
    term_cols = screen_width / FONT_W;
    term_rows = screen_height / FONT_H;
}

void gfx_scroll(void) {
    uint8_t *vram = (uint8_t *)(back_buffer ? back_buffer : video_memory);
    
    
    int row_bytes = screen_pitch * FONT_H;
    int copy_bytes = screen_pitch * (screen_height - FONT_H);
    
    fast_memcpy(vram, vram + row_bytes, copy_bytes);
    
    
    for(int y = screen_height - FONT_H; y < screen_height; y++) {
        for(int x = 0; x < screen_width; x++) {
            put_pixel(x, y, term_bg_color, 1);
        }
    }
}

void gfx_putc(unsigned int c) {
    if (term_cols == 0 || term_rows == 0) return; 

    if (c == '\n') {
        term_x = 0;
        term_y++;
    } 
    else if (c == '\t'){
        term_x += 4;
    }
    else if (c == '\b') {
        if (term_x > 0) term_x--;
        else if (term_y > 0) { term_x = term_cols - 1; term_y--; }
        
        for(int y=0; y<FONT_H; y++) {
            for(int x=0; x<FONT_W; x++) {
                put_pixel(term_x * FONT_W + x, term_y * FONT_H + y, term_bg_color, 1);
            }
        }
        
        vesa_render_rect(term_x * FONT_W, term_y * FONT_H, FONT_W, FONT_H);
    } 
    else if (c >= 32) {
        vesa_draw_char(term_x * FONT_W, term_y * FONT_H, c, term_fg_color, term_bg_color);
        
        vesa_render_rect(term_x * FONT_W, term_y * FONT_H, FONT_W, FONT_H);
        term_x++;
    }

    if (term_x >= term_cols) {
        term_x = 0;
        term_y++;
    }

    if (term_y >= term_rows) {
        gfx_scroll();
        vesa_render_rect(0, 0, screen_width, screen_height);
        term_y = term_rows - 1;
    }
}

void gfx_print(char *str) {
    while (*str) gfx_putc(*str++);
}

void gfx_set_color(uint32_t fg) { term_fg_color = fg; }
void gfx_set_bg_color(uint32_t bg) { term_bg_color = bg; }

void gfx_set_cursor(int x, int y) {
    if (x < 0) x = 0; if (x >= term_cols) x = term_cols - 1;
    if (y < 0) y = 0; if (y >= term_rows) y = term_rows - 1;
    term_x = x; term_y = y;
}

void gfx_get_cursor(int *x, int *y) { *x = term_x; *y = term_y; }