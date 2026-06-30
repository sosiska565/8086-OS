/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/apps/imgv.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include <oslib.h>
#include "libgui.h"

uint32_t *img_pixels = NULL;
int img_w = 0, img_h = 0;

int load_bmp(const char *path) {
    int sz = get_file_size(path);
    if (sz <= 54) return 0; 

    
    uint8_t *file_buf = malloc(sz);
    if (!file_buf) return 0;
    read_file(path, file_buf);

    
    if (file_buf[0] != 'B' || file_buf[1] != 'M') {
        free(file_buf);
        return 0;
    }

    uint32_t data_offset = *(uint32_t*)(&file_buf[10]);
    img_w = *(int32_t*)(&file_buf[18]);
    img_h = *(int32_t*)(&file_buf[22]);
    uint16_t bpp = *(uint16_t*)(&file_buf[28]);

    int top_down = 0;
    if (img_h < 0) {
        img_h = -img_h;
        top_down = 1;
    }

    
    if (bpp != 24 && bpp != 32) {
        free(file_buf);
        return 0; 
    }

    img_pixels = malloc(img_w * img_h * 4);
    if (!img_pixels) {
        free(file_buf);
        return 0;
    }

    int row_bytes = ((img_w * bpp + 31) / 32) * 4;

    
    for (int y = 0; y < img_h; y++) {
        int src_y = top_down ? y : (img_h - 1 - y);
        uint8_t *row_ptr = file_buf + data_offset + (src_y * row_bytes);
        
        for (int x = 0; x < img_w; x++) {
            uint8_t b = row_ptr[x * (bpp / 8) + 0];
            uint8_t g = row_ptr[x * (bpp / 8) + 1];
            uint8_t r = row_ptr[x * (bpp / 8) + 2];
            img_pixels[y * img_w + x] = (r << 16) | (g << 8) | b;
        }
    }
    
    free(file_buf);
    return 1;
}

void render_image(gui_window_t *win) {
    gui_draw_rect(win, 0, 0, win->w, win->h, 0x00181818); 

    if (!img_pixels) {
        gui_draw_string(win, 10, 10, "Failed to decode BMP.", 0xFFFFFF);
        return;
    }

    
    float scale_w = (float)win->w / img_w;
    float scale_h = (float)win->h / img_h;
    float scale = (scale_w < scale_h) ? scale_w : scale_h;

    int draw_w = (int)(img_w * scale);
    int draw_h = (int)(img_h * scale);
    
    int offset_x = (win->w - draw_w) / 2;
    int offset_y = (win->h - draw_h) / 2;

    
    for (int y = 0; y < draw_h; y++) {
        int src_y = (y * img_h) / draw_h;
        uint32_t *src_row = &img_pixels[src_y * img_w];
        
        for (int x = 0; x < draw_w; x++) {
            int src_x = (x * img_w) / draw_w;
            gui_put_pixel(win, offset_x + x, offset_y + y, src_row[src_x]);
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: imgv <path_to_bmp>\n");
        return 1;
    }

    gui_window_t *win = gui_create_window("Image Viewer", 600, 450);
    if (!win) return 1;

    
    gui_set_resizable(win, 1);

    int loaded = load_bmp(argv[1]);
    
    
    char* filename = strrchr(argv[1], '/');
    filename = filename ? filename + 1 : argv[1];

    if (loaded) {
        render_image(win);
        gui_render(win);
    }

    while (!win->closed) {
        gui_update(win);

        
        if (win->key_code || win->clicked || win->scroll_z != 0) {
            
        }

        render_image(win);
        gui_render(win);
        yield();
    }

    if (img_pixels) free(img_pixels);
    gui_destroy_window(win);
    return 0;
}
