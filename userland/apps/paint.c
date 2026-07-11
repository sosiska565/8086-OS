/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/apps/paint.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include <oslib.h>
#include "libgui.h"

#define MIN_WIN_W 400
#define MIN_WIN_H 300
#define MENU_H 24
#define TOOLBAR_H 40
#define CANVAS_Y (MENU_H + TOOLBAR_H)

uint32_t* canvas = NULL;
int canvas_w = 800;
int canvas_h = 600;
int scroll_x = 0;
int scroll_y = 0;

uint32_t palette[] = {
    0x00000000, 0x00FFFFFF, 0x00FF0000, 0x0000FF00, 
    0x000000FF, 0x00FFFF00, 0x0000FFFF, 0x00FF00FF, 
    0x00888888, 0x00C0C0C0, 0x00800000, 0x00008000,
    0x00000080, 0x00808000, 0x00008080, 0x00800080
};

int brush_size = 3;
uint32_t current_color = 0x00000000;
int active_menu = 0; 
char status_msg[64] = "";
int status_timer = 0;
int is_dragging_v = 0;
int is_dragging_h = 0;

int abs_val(int x) { return x < 0 ? -x : x; }

void show_status(const char* msg) {
    strcpy(status_msg, msg);
    status_timer = 150; 
}

void init_canvas(int w, int h) {
    if (canvas) free(canvas);
    canvas_w = w;
    canvas_h = h;
    canvas = malloc(w * h * 4);
    for(int i = 0; i < w * h; i++) canvas[i] = 0x00FFFFFF;
    scroll_x = 0; scroll_y = 0;
}

void draw_on_canvas(int cx, int cy, int radius, uint32_t color) {
    for(int y = -radius; y <= radius; y++) {
        for(int x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius) {
                int px = cx + x;
                int py = cy + y;
                if (px >= 0 && px < canvas_w && py >= 0 && py < canvas_h) {
                    canvas[py * canvas_w + px] = color;
                }
            }
        }
    }
}

int run_filepicker(char* out_path) {
    int pfd[2];
    pipe(pfd); 
    
    char* args[] = {"filepicker", NULL};
    int pid = spawn_ext("/path/filepicker.elf", args, -1, pfd[1]);
    close(pfd[1]); 

    if (pid < 0) {
        show_status("Error: Could not launch filepicker.elf!");
        return 0;
    }

    waitpid(pid); 

    int bytes = read(pfd[0], out_path, 255);
    close(pfd[0]);

    if (bytes > 0) {
        out_path[bytes] = '\0';
        return 1;
    }
    
    return 0; 
}

void load_canvas_from_bmp(const char* filepath) {
    int sz = get_file_size(filepath);
    if (sz <= 54) { show_status("Error: File not found!"); return; }
    uint8_t* bmp = malloc(sz);
    if (!bmp) return;
    read_file(filepath, bmp);
    
    if (bmp[0] != 'B' || bmp[1] != 'M') { show_status("Error: Not a BMP file!"); free(bmp); return; }
    
    uint32_t data_offset = *(uint32_t*)(&bmp[10]);
    int bmp_w = *(int32_t*)(&bmp[18]);
    int bmp_h = *(int32_t*)(&bmp[22]);
    uint16_t bpp = *(uint16_t*)(&bmp[28]);
    
    int top_down = 0;
    if (bmp_h < 0) { bmp_h = -bmp_h; top_down = 1; }
    
    if (bpp != 24 && bpp != 32) { show_status("Error: Only 24/32-bit BMP supported!"); free(bmp); return; }
    
    init_canvas(bmp_w, bmp_h);
    int row_bytes = ((bmp_w * bpp + 31) / 32) * 4;
    
    for(int y = 0; y < bmp_h && y < canvas_h; y++) {
        int src_y = top_down ? y : (bmp_h - 1 - y);
        uint8_t* row_ptr = bmp + data_offset + (src_y * row_bytes);
        for(int x = 0; x < bmp_w && x < canvas_w; x++) {
            uint8_t b = row_ptr[x * (bpp / 8) + 0];
            uint8_t g = row_ptr[x * (bpp / 8) + 1];
            uint8_t r = row_ptr[x * (bpp / 8) + 2];
            canvas[y * canvas_w + x] = (r << 16) | (g << 8) | b;
        }
    }
    free(bmp);
    show_status("Image loaded successfully!");
}

void save_canvas_to_bmp(const char* filepath) {
    int row_bytes = canvas_w * 3;
    if (row_bytes % 4 != 0) row_bytes += (4 - (row_bytes % 4)); 
    int filesize = 54 + (row_bytes * canvas_h);
    
    uint8_t* bmp = malloc(filesize);
    if (!bmp) return;
    
    memset(bmp, 0, filesize);
    bmp[0] = 'B'; bmp[1] = 'M';
    *(uint32_t*)(&bmp[2]) = filesize;
    *(uint32_t*)(&bmp[10]) = 54;
    *(uint32_t*)(&bmp[14]) = 40;
    *(int32_t*)(&bmp[18]) = canvas_w;
    *(int32_t*)(&bmp[22]) = -canvas_h;
    *(uint16_t*)(&bmp[26]) = 1;
    *(uint16_t*)(&bmp[28]) = 24;
    *(uint32_t*)(&bmp[34]) = row_bytes * canvas_h;
    
    for(int y = 0; y < canvas_h; y++) {
        uint8_t* row_ptr = bmp + 54 + (y * row_bytes);
        int ptr = 0;
        for(int x = 0; x < canvas_w; x++) {
            uint32_t pixel = canvas[y * canvas_w + x];
            row_ptr[ptr++] = pixel & 0xFF;
            row_ptr[ptr++] = (pixel >> 8) & 0xFF;
            row_ptr[ptr++] = (pixel >> 16) & 0xFF;
        }
    }
    
    if (write_file(filepath, bmp, filesize) > 0) show_status("Saved successfully!");
    else show_status("Error: Write failed!");
    free(bmp);
}

int main(int argc, char** argv) {
    gui_window_t* win = gui_create_window("Paint", 720, 500);
    if (!win) return 1;
    gui_set_resizable(win, 1);
    init_canvas(800, 600); 

    int prev_mx = -1, prev_my = -1;

    while(!win->closed) {
        gui_update(win);
        int mx = win->mx;
        int my = win->my;
        int clicked = win->clicked;
        int mbtn_down = win->mbtn & 1;

        int max_scroll_y = canvas_h - (win->h - CANVAS_Y);
        int max_scroll_x = canvas_w - win->w;
        
        int sb_thick = 14;
        if (max_scroll_x > 0) max_scroll_y += sb_thick; 
        if (max_scroll_y > 0) max_scroll_x += sb_thick;
        
        if (max_scroll_x < 0) max_scroll_x = 0;
        if (max_scroll_y < 0) max_scroll_y = 0;

        if (scroll_x > max_scroll_x) scroll_x = max_scroll_x;
        if (scroll_y > max_scroll_y) scroll_y = max_scroll_y;

        if (mbtn_down && clicked && active_menu == 0) {
            if (max_scroll_y > 0 && mx >= win->w - sb_thick && my >= CANVAS_Y && my <= win->h - (max_scroll_x > 0 ? sb_thick : 0)) is_dragging_v = 1;
            if (max_scroll_x > 0 && my >= win->h - sb_thick && mx >= 0 && mx <= win->w - (max_scroll_y > 0 ? sb_thick : 0)) is_dragging_h = 1;
        }
        if (!mbtn_down) { is_dragging_v = 0; is_dragging_h = 0; }

        if (is_dragging_v && max_scroll_y > 0) {
            int track_h = win->h - CANVAS_Y - (max_scroll_x > 0 ? sb_thick : 0);
            int thumb_h = (track_h * track_h) / (canvas_h + track_h);
            if (thumb_h < 20) thumb_h = 20;
            int rel_y = my - CANVAS_Y - (thumb_h / 2);
            if (rel_y < 0) rel_y = 0;
            if (rel_y > track_h - thumb_h) rel_y = track_h - thumb_h;
            scroll_y = (rel_y * max_scroll_y) / (track_h - thumb_h);
        }

        if (is_dragging_h && max_scroll_x > 0) {
            int track_w = win->w - (max_scroll_y > 0 ? sb_thick : 0);
            int thumb_w = (track_w * track_w) / (canvas_w + track_w);
            if (thumb_w < 20) thumb_w = 20;
            int rel_x = mx - (thumb_w / 2);
            if (rel_x < 0) rel_x = 0;
            if (rel_x > track_w - thumb_w) rel_x = track_w - thumb_w;
            scroll_x = (rel_x * max_scroll_x) / (track_w - thumb_w);
        }

        if (win->scroll_z != 0) {
            if (max_scroll_y > 0) {
                scroll_y += win->scroll_z * 30;
                if (scroll_y < 0) scroll_y = 0;
                if (scroll_y > max_scroll_y) scroll_y = max_scroll_y;
            } else {
                brush_size += win->scroll_z;
                if (brush_size < 1) brush_size = 1;
                if (brush_size > 50) brush_size = 50;
            }
        }

        int in_canvas_area = (my > CANVAS_Y && my < win->h - (max_scroll_x > 0 ? sb_thick : 0) && mx < win->w - (max_scroll_y > 0 ? sb_thick : 0));
        int can_paint = (in_canvas_area && active_menu == 0 && !is_dragging_v && !is_dragging_h);

        if (can_paint) {
            if (mbtn_down) {
                int cx = mx + scroll_x;
                int cy = my - CANVAS_Y + scroll_y;

                if (prev_mx != -1 && prev_my != -1) {
                    int dx = cx - prev_mx;
                    int dy = cy - prev_my;
                    int steps = abs_val(dx) > abs_val(dy) ? abs_val(dx) : abs_val(dy);
                    if (steps == 0) steps = 1;
                    for(int i = 0; i <= steps; i++) draw_on_canvas(prev_mx + (dx * i) / steps, prev_my + (dy * i) / steps, brush_size, current_color);
                } else {
                    draw_on_canvas(cx, cy, brush_size, current_color);
                }
                prev_mx = cx;
                prev_my = cy;
            } else { prev_mx = -1; prev_my = -1; }
        } else { prev_mx = -1; prev_my = -1; }

        int view_w = win->w - (max_scroll_y > 0 ? sb_thick : 0);
        int view_h = win->h - CANVAS_Y - (max_scroll_x > 0 ? sb_thick : 0);

        for(int y = 0; y < view_h; y++) {
            for(int x = 0; x < view_w; x++) {
                int cx = x + scroll_x;
                int cy = y + scroll_y;
                uint32_t color = 0x002D2D30; 
                if (cx >= 0 && cx < canvas_w && cy >= 0 && cy < canvas_h) color = canvas[cy * canvas_w + cx];
                gui_put_pixel(win, x, y + CANVAS_Y, color);
            }
        }

        if (max_scroll_y > 0) {
            gui_draw_rect(win, win->w - sb_thick, CANVAS_Y, sb_thick, win->h - CANVAS_Y, 0x00E0E0E0);
            int track_h = view_h;
            int thumb_h = (track_h * track_h) / (canvas_h + track_h);
            if (thumb_h < 20) thumb_h = 20;
            int thumb_y = CANVAS_Y + (scroll_y * (track_h - thumb_h)) / max_scroll_y;
            gui_draw_rounded_rect(win, win->w - sb_thick + 2, thumb_y, sb_thick - 4, thumb_h, 3, is_dragging_v ? 0x00888888 : 0x00A0A0A0);
        }
        
        if (max_scroll_x > 0) {
            gui_draw_rect(win, 0, win->h - sb_thick, win->w - (max_scroll_y > 0 ? sb_thick : 0), sb_thick, 0x00E0E0E0);
            int track_w = view_w;
            int thumb_w = (track_w * track_w) / (canvas_w + track_w);
            if (thumb_w < 20) thumb_w = 20;
            int thumb_x = (scroll_x * (track_w - thumb_w)) / max_scroll_x;
            gui_draw_rounded_rect(win, thumb_x, win->h - sb_thick + 2, thumb_w, sb_thick - 4, 3, is_dragging_h ? 0x00888888 : 0x00A0A0A0);
        }

        if (max_scroll_x > 0 && max_scroll_y > 0) gui_draw_rect(win, win->w - sb_thick, win->h - sb_thick, sb_thick, sb_thick, 0x00C0C0C0);

        gui_draw_rect(win, 0, 0, win->w, MENU_H, 0x00E8E8E8);
        gui_draw_rect(win, 0, MENU_H - 1, win->w, 1, 0x00A0A0A0);

        int file_hov = (mx >= 5 && mx <= 45 && my >= 0 && my <= MENU_H);
        int opt_hov  = (mx >= 50 && mx <= 110 && my >= 0 && my <= MENU_H);

        gui_draw_rect(win, 5, 0, 40, MENU_H, (active_menu==1 || (file_hov && mbtn_down)) ? 0x000A84FF : 0x00E8E8E8);
        gui_draw_string(win, 12, 5, "File", (active_menu==1 || (file_hov && mbtn_down)) ? 0x00FFFFFF : 0x00000000);

        gui_draw_rect(win, 50, 0, 60, MENU_H, (active_menu==2 || (opt_hov && mbtn_down)) ? 0x000A84FF : 0x00E8E8E8);
        gui_draw_string(win, 55, 5, "Options", (active_menu==2 || (opt_hov && mbtn_down)) ? 0x00FFFFFF : 0x00000000);

        if (clicked) {
            if (file_hov) active_menu = (active_menu == 1) ? 0 : 1;
            else if (opt_hov) active_menu = (active_menu == 2) ? 0 : 2;
            else {
                int in_file_dd = (active_menu == 1 && mx >= 5 && mx <= 125 && my >= MENU_H && my <= MENU_H + 95);
                int in_opt_dd  = (active_menu == 2 && mx >= 50 && mx <= 190 && my >= MENU_H && my <= MENU_H + 75);
                if (!in_file_dd && !in_opt_dd) active_menu = 0; 
            }
        }

        gui_draw_rect(win, 0, MENU_H, win->w, TOOLBAR_H, 0x00F0F0F0);
        gui_draw_rect(win, 0, CANVAS_Y - 1, win->w, 1, 0x00A0A0A0);

        int num_colors = sizeof(palette) / sizeof(uint32_t);
        for(int i = 0; i < num_colors; i++) {
            int cx = 10 + (i % 8) * 28;
            int cy = MENU_H + 4 + (i / 8) * 16;
            
            if (clicked && active_menu == 0) {
                if (mx >= cx && mx <= cx + 24 && my >= cy && my <= cy + 14) current_color = palette[i];
            }

            if (current_color == palette[i]) gui_draw_rect(win, cx - 2, cy - 2, 28, 18, 0x00007AFF);
            else gui_draw_rect(win, cx - 1, cy - 1, 26, 16, 0x00555555);
            gui_draw_rect(win, cx, cy, 24, 14, palette[i]);
        }

        char info_str[128];
        sprintf(info_str, "Brush: %d px | Canvas: %dx%d", brush_size, canvas_w, canvas_h);
        gui_draw_string(win, 260, MENU_H + 12, info_str, 0x00000000);

        if (status_timer > 0) {
            gui_draw_string(win, win->w - 200, MENU_H + 12, status_msg, 0x0034C759);
            status_timer--;
        }

        if (active_menu == 1) { 
            gui_draw_rect(win, 5, MENU_H, 130, 95, 0x00FFFFFF);
            gui_draw_rect(win, 5, MENU_H, 130, 95, 0x00A0A0A0);
            gui_draw_rect(win, 6, MENU_H, 128, 94, 0x00FFFFFF); 

            if (gui_button(win, 10, MENU_H + 5, 120, 20, "New Canvas", 0)) {
                init_canvas(800, 600);
                active_menu = 0;
            }
            if (gui_button(win, 10, MENU_H + 28, 120, 20, "Open BMP...", 0)) {
                active_menu = 0;
                char selected_path[256];
                if (run_filepicker(selected_path)) {
                    load_canvas_from_bmp(selected_path);
                }
            }
            if (gui_button(win, 10, MENU_H + 51, 120, 20, "Save As...", 0)) {
                active_menu = 0;
                char selected_path[256];
                if (run_filepicker(selected_path)) {
                    save_canvas_to_bmp(selected_path);
                }
            }
            if (gui_button(win, 10, MENU_H + 74, 120, 20, "Exit", 0)) {
                win->closed = 1;
            }
        } 
        else if (active_menu == 2) { 
            gui_draw_rect(win, 50, MENU_H, 140, 75, 0x00FFFFFF);
            gui_draw_rect(win, 50, MENU_H, 140, 75, 0x00A0A0A0);
            gui_draw_rect(win, 51, MENU_H, 138, 74, 0x00FFFFFF);

            if (gui_button(win, 55, MENU_H + 5, 130, 20, "Clear Canvas", 0)) {
                for(int i = 0; i < canvas_w * canvas_h; i++) canvas[i] = 0x00FFFFFF;
                active_menu = 0;
            }
            if (gui_button(win, 55, MENU_H + 28, 130, 20, "Brush Size +", 0)) {
                if (brush_size < 50) brush_size++;
            }
            if (gui_button(win, 55, MENU_H + 51, 130, 20, "Brush Size -", 0)) {
                if (brush_size > 1) brush_size--;
            }
        }

        gui_render(win);
        yield();
    }

    if (canvas) free(canvas);
    gui_destroy_window(win);
    return 0;
}
