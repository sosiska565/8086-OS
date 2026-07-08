/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/apps/fonttest.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include <oslib.h>
#include "libgui.h"

#include "stb_truetype.h"

extern float g_ui_scale;

static inline uint32_t blend_pixel(uint32_t fg, uint32_t bg, uint8_t alpha) {
    if (alpha == 0) return bg;
    if (alpha == 255) return fg;
    uint32_t rb = bg & 0x00FF00FF;
    uint32_t g  = bg & 0x0000FF00;
    rb += (((fg & 0x00FF00FF) - rb) * alpha) >> 8;
    g  += (((fg & 0x0000FF00) - g ) * alpha) >> 8;
    return (rb & 0x00FF00FF) | (g & 0x0000FF00);
}

int main(int argc, char** argv) {
    int sz = get_file_size("/system/font.ttf");
    if (sz <= 0) {
        printf("Error: /system/font.ttf not found!\n");
        return 1;
    }
    
    uint8_t* ttf_buffer = malloc(sz);
    read_file("/system/font.ttf", ttf_buffer);
    
    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, ttf_buffer, 0)) {
        printf("Error: Failed to initialize TTF font!\n");
        free(ttf_buffer);
        return 1;
    }

    gui_window_t* win = gui_create_window("Dynamic Font Scaler", 640, 360);
    if (!win) return 1;
    gui_set_resizable(win, 1);

    int last_w = 0, last_h = 0;

    while (!win->closed) {
        gui_update(win);

        if (win->w != last_w || win->h != last_h) {
            last_w = win->w;
            last_h = win->h;

            gui_draw_rect(win, 0, 0, win->w, win->h, 0x001E1E1E);

            int real_w = (int)(win->w * g_ui_scale);
            int real_h = (int)(win->h * g_ui_scale);

            int cols = 16;
            int rows = 6;
            float cell_w = (float)real_w / cols;
            float cell_h = (float)real_h / rows;

            float target_size = (cell_w < cell_h ? cell_w : cell_h) * 0.7f;
            if (target_size < 1.0f) target_size = 1.0f; 

            float scale = stbtt_ScaleForPixelHeight(&font, target_size);
            int ascent, descent, lineGap;
            stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
            ascent = (int)(ascent * scale);

            for (int i = 0; i < 95; i++) {
                char c = 32 + i; 
                
                int grid_x = i % cols;
                int grid_y = i / cols;

                int px = (int)(grid_x * cell_w);
                int py = (int)(grid_y * cell_h);

                uint32_t cell_bg = (grid_x + grid_y) % 2 == 0 ? 0x00252526 : 0x002D2D30;
                for (int cy = 0; cy < (int)cell_h; cy++) {
                    for (int cx = 0; cx < (int)cell_w; cx++) {
                        if (px + cx < real_w && py + cy < real_h) {
                            win->backbuffer[(py + cy) * real_w + (px + cx)] = cell_bg;
                        }
                    }
                }

                int advance, lsb, bw, bh, bxoff, byoff;
                stbtt_GetCodepointHMetrics(&font, c, &advance, &lsb);
                unsigned char *bitmap = stbtt_GetCodepointBitmap(&font, 0, scale, c, &bw, &bh, &bxoff, &byoff);

                if (bitmap) {
                    int char_w_scaled = (int)(advance * scale);
                    int char_x = px + ((int)cell_w - char_w_scaled) / 2 + bxoff;
                    
                    int char_y = py + ((int)cell_h - (int)target_size) / 2 + ascent + byoff;

                    for (int by = 0; by < bh; by++) {
                        for (int bx = 0; bx < bw; bx++) {
                            uint8_t alpha = bitmap[by * bw + bx];
                            if (alpha > 0) {
                                int final_x = char_x + bx;
                                int final_y = char_y + by;

                                if (final_x >= 0 && final_x < real_w && final_y >= 0 && final_y < real_h) {
                                    uint32_t bg = win->backbuffer[final_y * real_w + final_x];
                                    uint32_t fg = (grid_x % 2 == 0) ? 0x000A84FF : 0x0034C759;
                                    win->backbuffer[final_y * real_w + final_x] = blend_pixel(fg, bg, alpha);
                                }
                            }
                        }
                    }
                    stbtt_FreeBitmap(bitmap, NULL); 
                }
            }
            gui_render(win); 
        }

        yield(); 
    }

    free(ttf_buffer);
    gui_destroy_window(win);
    return 0;
}
