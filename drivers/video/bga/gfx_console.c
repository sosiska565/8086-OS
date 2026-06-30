/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/drivers/video/bga/gfx_console.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include "drivers/video/bga/gfx_console.h"
#include "drivers/video/vesa.h"
#include "mm/memory.h"
#include "drivers/vga/vga.h"
#include "utils/utils.h"
#include "kernel/include/string.h" 

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
extern int buffer_size_bytes;
extern uint32_t *video_memory;
extern uint32_t *back_buffer;

int gfx_batch_mode = 0;
int gfx_is_dirty = 0;
int gfx_dirty_min_y = 99999;
int gfx_dirty_max_y = -1;

void gfx_start_batch(void) {
    gfx_batch_mode = 1;
    gfx_is_dirty = 0;
    gfx_dirty_min_y = 99999;
    gfx_dirty_max_y = -1;
}

void gfx_end_batch(void) {
    gfx_batch_mode = 0;
    if (!back_buffer || !video_memory) return;

    if (gfx_is_dirty) {
        
        vesa_render_buffer(); 
    } else if (gfx_dirty_max_y >= 0) {
        
        int h = (gfx_dirty_max_y - gfx_dirty_min_y + 1) * FONT_H;
        vesa_render_rect(0, gfx_dirty_min_y * FONT_H, screen_width, h);
    }
}

void init_gfx_console(void) {
    term_x = 0;
    term_y = 0;
    term_cols = screen_width / FONT_W;
    term_rows = screen_height / FONT_H;
}

void gfx_scroll(void) {
    uint8_t *vram = (uint8_t *)back_buffer; 
    if (!vram) return; 
    
    int row_bytes = screen_pitch * FONT_H;
    int copy_bytes = screen_pitch * (screen_height - FONT_H);
    
    if (copy_bytes <= 0) return;

    
    memmove(vram, vram + row_bytes, copy_bytes);
    
    
    uint32_t *bottom_line = (uint32_t *)(vram + copy_bytes);
    int pixels = (screen_pitch * FONT_H) / 4;
    for (int i = 0; i < pixels; i++) {
        bottom_line[i] = term_bg_color;
    }

    if (gfx_batch_mode) {
        gfx_is_dirty = 1; 
    } else {
        vesa_render_buffer();
    }
}

static int ansi_state = 0;
static char ansi_buf[64];
static int ansi_pos = 0;

void gfx_putc(unsigned int c) {
    if (term_cols == 0 || term_rows == 0 || !back_buffer) return; 

    if (ansi_state == 1) {
        if (c == '[') { ansi_state = 2; ansi_pos = 0; ansi_buf[0] = '\0'; }
        else ansi_state = 0;
        return;
    } else if (ansi_state == 2) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
            ansi_buf[ansi_pos] = '\0';
            
            if (c == 'J') { 
                if (ansi_pos == 0 || strcmp(ansi_buf, "2") == 0) {
                    int total_pixels = buffer_size_bytes / 4;
                    uint32_t *buf32 = (uint32_t *)back_buffer;
                    for (int i = 0; i < total_pixels; i++) buf32[i] = term_bg_color;
                    term_x = 0; term_y = 0;
                    if (gfx_batch_mode) gfx_is_dirty = 1; else vesa_render_buffer();
                }
            } 
            else if (c == 'H' || c == 'f') { 
                int r = 1, col = 1;
                if (ansi_pos > 0) {
                    char *semicolon = strchr(ansi_buf, ';');
                    if (semicolon) {
                        *semicolon = '\0';
                        r = atoi(ansi_buf, 10);
                        col = atoi(semicolon + 1, 10);
                    } else {
                        r = atoi(ansi_buf, 10);
                    }
                }
                if (r < 1) r = 1; if (col < 1) col = 1;
                term_y = r - 1; term_x = col - 1;
                if (term_y >= term_rows) term_y = term_rows - 1;
                if (term_x >= term_cols) term_x = term_cols - 1;
            }
            else if (c == 'A') { int arg = atoi(ansi_buf, 10); term_y -= (arg == 0 ? 1 : arg); if (term_y < 0) term_y = 0; }
            else if (c == 'B') { int arg = atoi(ansi_buf, 10); term_y += (arg == 0 ? 1 : arg); if (term_y >= term_rows) term_y = term_rows - 1; }
            else if (c == 'C') { int arg = atoi(ansi_buf, 10); term_x += (arg == 0 ? 1 : arg); if (term_x >= term_cols) term_x = term_cols - 1; }
            else if (c == 'D') { int arg = atoi(ansi_buf, 10); term_x -= (arg == 0 ? 1 : arg); if (term_x < 0) term_x = 0; }
            else if (c == 'm') { 
                if (ansi_pos == 0) {
                    term_fg_color = 0x00FFFFFF;
                    term_bg_color = 0x00000000;
                } else {
                    char *p = ansi_buf;
                    while (*p) {
                        int code = atoi(p, 10);
                        if (code == 0) { term_fg_color = 0x00FFFFFF; term_bg_color = 0x00000000; }
                        else if (code >= 30 && code <= 37) term_fg_color = vga_to_rgb[code - 30];
                        else if (code >= 40 && code <= 47) term_bg_color = vga_to_rgb[code - 40];
                        else if (code >= 90 && code <= 97) term_fg_color = vga_to_rgb[code - 90 + 8];
                        else if (code >= 100 && code <= 107) term_bg_color = vga_to_rgb[code - 100 + 8];
                        
                        while (*p && *p != ';') p++;
                        if (*p == ';') p++;
                    }
                }
            }
            ansi_state = 0;
        } else if (ansi_pos < 63) {
            ansi_buf[ansi_pos++] = c;
        } else {
            ansi_state = 0; 
        }
        return;
    }

    if (c == 27) { ansi_state = 1; return; }

    if (c == '\n') { term_x = 0; term_y++; }
    else if (c == '\r') { term_x = 0; }
    else if (c == '\t'){ term_x += 4; }
    else if (c == '\b') {
        if (term_x > 0) term_x--;
        else if (term_y > 0) { term_x = term_cols - 1; term_y--; }
        
        for(int y = 0; y < 8; y++) {
            for(int x = 0; x < 8; x++) {
                uint8_t *pixel_addr = (uint8_t*)back_buffer + ((term_y * 8 + y) * screen_pitch) + ((term_x * 8 + x) * 4);
                *(uint32_t*)pixel_addr = term_bg_color;
            }
        }
        
        if (gfx_batch_mode) {
            if (term_y < gfx_dirty_min_y) gfx_dirty_min_y = term_y;
            if (term_y > gfx_dirty_max_y) gfx_dirty_max_y = term_y;
        } else {
            vesa_render_rect(term_x * 8, term_y * 8, 8, 8);
        }
    } 
    else if (c >= 32) {
        vesa_draw_char(term_x * 8, term_y * 8, c, term_fg_color, term_bg_color);
        
        if (gfx_batch_mode) {
            if (term_y < gfx_dirty_min_y) gfx_dirty_min_y = term_y;
            if (term_y > gfx_dirty_max_y) gfx_dirty_max_y = term_y;
        } else {
            vesa_render_rect(term_x * 8, term_y * 8, 8, 8);
        }
        term_x++;
    }

    if (term_x >= term_cols) { term_x = 0; term_y++; }
    if (term_y >= term_rows) { gfx_scroll(); term_y = term_rows - 1; }
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
