#include "drivers/video/graphics.h"
#include "drivers/video/vesa.h"
#include "drivers/video/bga/font.h"
#include "drivers/vga/vga.h"
#include "memory/memory.h"
#include "multitask/task.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "global.h"

#define FONT_W 8
#define FONT_H 8
#define WINDOW_RADIUS 10

#define ANIM_SPEED_OPEN  5
#define ANIM_SPEED_CLOSE 5 
#define ANIM_SPEED_FS    2
#define ANIM_STEPS_WS    30


static int abs(int i) { return i < 0 ? -i : i; }

Window *current_output_window = 0;
Window *head = 0;
int window_count = 0;
int next_id = 1;
Window *focused_window = 0;
char *focused_task;

int global_anim_x = 0; 

int is_window_visible(Window *win) {
    if (!win) return 1;
    if (win->workspace != current_workspace) return 0;
    return 1;
}

void wm_set_focused_window(Window *win) { focused_window = win; }

void set_current_output_window(Window *win) {
    if (current_task) current_task->window = win;
    current_output_window = win;
    focused_window = win;
}

void draw_rect(int x, int y, int w, int h, uint32_t color) {
    for (int i = 0; i < w; i++) { put_pixel(x + i, y, color, 1); put_pixel(x + i, y + h - 1, color, 1); }
    for (int i = 0; i < h; i++) { put_pixel(x, y + i, color, 1); put_pixel(x + w - 1, y + i, color, 1); }
}

void draw_rect_filled(int x, int y, int w, int h, uint32_t color) {
    extern int screen_bpp;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    int sw = get_screen_width();
    int sh = get_screen_height();
    if (x + w > sw) w = sw - x;
    if (y + h > sh) h = sh - y;
    if (w <= 0 || h <= 0) return;

    uint8_t bytes_per_pixel = screen_bpp / 8;
    uint8_t* base = (back_buffer != 0) ? (uint8_t*)back_buffer : (uint8_t*)video_memory;

    for (int i = 0; i < h; i++) {
        uint8_t* pixel_ptr = base + ((y + i) * screen_pitch) + (x * bytes_per_pixel);
        if (bytes_per_pixel == 4) {
            fast_memset(pixel_ptr, color, w); 
        } else {
            for (int j = 0; j < w; j++) {
                pixel_ptr[0] = color & 0xFF;
                pixel_ptr[1] = (color >> 8) & 0xFF;
                pixel_ptr[2] = (color >> 16) & 0xFF;
                pixel_ptr += bytes_per_pixel;
            }
        }
    }
}

void draw_line(int x1, int y1, int x2, int y2, uint32_t color) {
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy;
    while (1) {
        put_pixel(x1, y1, color, 1);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

void draw_circle(int x0, int y0, int radius, uint32_t color) {
    int x = 0, y = radius, d = 3 - 2 * radius;
    while (y >= x) {
        put_pixel(x0 + x, y0 + y, color, 1); put_pixel(x0 - x, y0 + y, color, 1);
        put_pixel(x0 + x, y0 - y, color, 1); put_pixel(x0 - x, y0 - y, color, 1);
        put_pixel(x0 + y, y0 + x, color, 1); put_pixel(x0 - y, y0 + x, color, 1);
        put_pixel(x0 + y, y0 - x, color, 1); put_pixel(x0 - y, y0 - x, color, 1);
        x++;
        if (d > 0) { y--; d = d + 4 * (x - y) + 10; } else { d = d + 4 * x + 6; }
    }
}

void draw_circle_filled(int x0, int y0, int radius, uint32_t color) {
    int x = 0, y = radius, d = 3 - 2 * radius;
    while (y >= x) {
        draw_line(x0 - x, y0 + y, x0 + x, y0 + y, color); draw_line(x0 - x, y0 - y, x0 + x, y0 - y, color);
        draw_line(x0 - y, y0 + x, x0 + y, y0 + x, color); draw_line(x0 - y, y0 - x, x0 + y, y0 - x, color);
        x++;
        if (d > 0) { y--; d = d + 4 * (x - y) + 10; } else { d = d + 4 * x + 6; }
    }
}

static int get_circle_margin(int r, int y) {
    int dx = r;
    while (dx*dx + y*y > r*r && dx > 0) dx--;
    return r - dx;
}

void draw_rounded_rect_filled(int x, int y, int w, int h, int r, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    if (r > w/2) r = w/2; if (r > h/2) r = h/2;
    if (r <= 0) { draw_rect_filled(x, y, w, h, color); return; }

    for (int i = 0; i < h; i++) {
        int start_x = x; int row_w = w;
        if (i < r) {
            int margin = get_circle_margin(r, r - i - 1);
            start_x += margin; row_w -= margin * 2;
        } else if (i >= h - r) {
            int margin = get_circle_margin(r, i - (h - r));
            start_x += margin; row_w -= margin * 2;
        }
        draw_rect_filled(start_x, y + i, row_w, 1, color);
    }
}

void window_draw_rect_filled(Window *win, int local_x, int local_y, int w, int h, uint32_t color) {
    if (!win || win->workspace != current_workspace) return; 

    if (local_x < 0) { w += local_x; local_x = 0; }
    if (local_y < 0) { h += local_y; local_y = 0; }
    if (local_x + w > win->width) w = win->width - local_x;
    if (local_y + h > win->height) h = win->height - local_y;
    if (w <= 0 || h <= 0) return;

    int abs_x = win->x + local_x;
    int abs_y = win->y + local_y;
    int r = WINDOW_RADIUS;

    for (int i = 0; i < h; i++) {
        int draw_x = abs_x;
        int draw_w = w;
        int current_ly = local_y + i;
        int margin = 0;

        if (current_ly < r) margin = get_circle_margin(r, r - current_ly - 1);
        else if (current_ly >= win->height - r) margin = get_circle_margin(r, current_ly - (win->height - r));

        if (margin > 0) {
            if (local_x < margin) { int diff = margin - local_x; draw_x += diff; draw_w -= diff; }
            int local_end = local_x + w;
            int max_end = win->width - margin;
            if (local_end > max_end) draw_w -= (local_end - max_end);
        }
        if (draw_w > 0) draw_rect_filled(draw_x, abs_y + i, draw_w, 1, color);
    }
}

void window_draw_char(Window *win, int local_x, int local_y, unsigned int c, uint32_t color) {
    if (!win || win->workspace != current_workspace) return;
    
    uint8_t *glyph = (uint8_t*)font8x8_basic[c];
    extern int screen_bpp; 
    uint8_t bpp = screen_bpp / 8;
    uint8_t* buffer = (back_buffer != 0) ? (uint8_t*)back_buffer : (uint8_t*)video_memory;
    
    int r = WINDOW_RADIUS;

    for (int row = 0; row < 8; row++) {
        int current_ly = local_y + row;
        if (current_ly < 0 || current_ly >= win->height) continue;

        int abs_y = win->y + current_ly;
        int margin = 0;

        if (current_ly < r) {
            margin = get_circle_margin(r, r - current_ly - 1);
        } else if (current_ly >= win->height - r) {
            margin = get_circle_margin(r, current_ly - (win->height - r));
        }

        uint8_t* dest = buffer + (abs_y * screen_pitch) + ((win->x + local_x) * bpp);
        uint8_t font_row = glyph[row];
        
        for (int col = 0; col < 8; col++) {
            int current_lx = local_x + col;
            
            if (current_lx >= margin && current_lx < win->width - margin) {
                if ((font_row >> col) & 1) {
                    if (bpp == 4) *(uint32_t*)dest = color;
                    else { dest[0] = color; dest[1] = color>>8; dest[2] = color>>16; }
                }
            }
            dest += bpp;
        }
    }
}

void buffer_write(Window *win, int col, int row, unsigned int c, uint32_t color) {
    int max_cols = get_screen_width() / FONT_W;
    if (!win->char_buffer || !win->color_buffer) return;
    int idx = row * max_cols + col;
    win->char_buffer[idx] = c;
    win->color_buffer[idx] = color;
}

void window_redraw_content(Window *win) {
    if (!win || !win->char_buffer || win->workspace != current_workspace) return;
    int max_cols = get_screen_width() / FONT_W;
    for (int r = 0; r < win->rows; r++) {
        for (int c = 0; c < win->cols; c++) {
            int idx = r * max_cols + c;
            unsigned int symb = win->char_buffer[idx];
            uint32_t color = win->color_buffer[idx];
            if (symb != 0 && symb != ' ') window_draw_char(win, c * 8, r * 8, symb, color);
        }
    }
}

void window_clear(Window *win, uint32_t color){
    if (is_window_visible(win)) draw_rounded_rect_filled(win->x, win->y, win->width, win->height, WINDOW_RADIUS, color);
    int total = (get_screen_width() / FONT_W) * (get_screen_height() / FONT_H);
    if (win->char_buffer) {
        for(int i = 0; i < total; i++) { win->char_buffer[i] = L' '; win->color_buffer[i] = color; }
    }
    win->cursor_x = 0; win->cursor_y = 0;
    wm_render_window(win);
}

void window_scroll(Window *win){
    if (is_window_visible(win)) draw_rounded_rect_filled(win->x, win->y, win->width, win->height, WINDOW_RADIUS, win->bg_color);
    int max_cols = get_screen_width() / FONT_W;
    if (win->char_buffer) {
        for (int r = 0; r < win->rows - 1; r++) {
            for (int c = 0; c < win->cols; c++) {
                int current = r * max_cols + c;
                int next = (r + 1) * max_cols + c;
                win->char_buffer[current] = win->char_buffer[next];
                win->color_buffer[current] = win->color_buffer[next];
            }
        }
        int last_row_start = (win->rows - 1) * max_cols;
        for (int c = 0; c < win->cols; c++) {
            win->char_buffer[last_row_start + c] = ' ';
            win->color_buffer[last_row_start + c] = win->bg_color;
        }
    }
    win->cursor_x = 0; win->cursor_y = (win->rows - 1) * FONT_H;
    if (is_window_visible(win)) window_redraw_content(win);
}

void window_print(Window *win, int x, int y, char *str, uint32_t color) {
    int old_x = win->cursor_x; int old_y = win->cursor_y; uint32_t old_c = win->text_color;
    win->cursor_x = x; win->cursor_y = y; win->text_color = color;
    while (*str) { window_putc(win, *str); str++; }
    win->cursor_x = old_x; win->cursor_y = old_y; win->text_color = old_c;
}

void window_render_char_rect(Window *win, int col, int row) {
    if (!is_window_visible(win)) return;
    vesa_render_rect(win->x + col * 8, win->y + row * 8, 8, 8);
}

void window_putc(Window *win, unsigned int c) {
    if (win == 0) return;
    int col_idx = win->cursor_x / 8; int row_idx = win->cursor_y / 8;

    if (c == '\n') { win->cursor_x = 0; win->cursor_y += 8; } 
    else if (c == '\b') {
        if (win->cursor_x >= 8) win->cursor_x -= 8;
        else if (win->cursor_y >= 8) { win->cursor_x = (win->cols - 1) * 8; win->cursor_y -= 8; }
        else return;
        
        col_idx = win->cursor_x / 8; row_idx = win->cursor_y / 8;
        buffer_write(win, col_idx, row_idx, ' ', win->bg_color);
        if (is_window_visible(win)) { 
            window_draw_rect_filled(win, win->cursor_x, win->cursor_y, 8, 8, win->bg_color);
            window_render_char_rect(win, col_idx, row_idx);
        }
    } else {
        buffer_write(win, col_idx, row_idx, c, win->text_color);
        if (is_window_visible(win)) {
            window_draw_char(win, win->cursor_x, win->cursor_y, c, win->text_color);
            window_render_char_rect(win, col_idx, row_idx);
        }
        win->cursor_x += 8;
    }

    if (win->cursor_x >= win->width - 8) { win->cursor_x = 0; win->cursor_y += 8; }
    if (win->cursor_y >= win->height - 8) {
        window_scroll(win);
        if (is_window_visible(win)) vesa_render_rect(win->x, win->y, win->width, win->height);
    }
}




void wm_init(){ head = 0; window_count = 0; }

void draw_window(Window *win, int x, int y, int w, int h, uint32_t color_frame, uint32_t color_bg, int centered) {
    draw_rounded_rect_filled(x - 2, y - 2, w + 4, h + 4, WINDOW_RADIUS + 2, color_frame);
    draw_rounded_rect_filled(x, y, w, h, WINDOW_RADIUS, color_bg);
}

void wm_render_window(Window *win) {
    if (!is_window_visible(win)) return;
    int rx = win->x - 5; if (rx < 0) rx = 0;
    int ry = win->y - 5; if (ry < TASKBAR_HEIGHT) ry = TASKBAR_HEIGHT;
    vesa_render_rect(rx, ry, win->width + 10, win->height + 10);
}

void wm_animate_open(Window *win) {
    for(int p = 20; p <= 100; p += ANIM_SPEED_OPEN) { win->anim_scale = p; wm_refresh(); }
    win->anim_scale = 100; wm_refresh();
}

void wm_animate_close(Window *win) {
    for(int p = 100; p >= 20; p -= ANIM_SPEED_CLOSE) { win->anim_scale = p; wm_refresh(); }
}

void wm_toggle_fullscreen() {
    if (!focused_window) return;
    for(int p = 100; p >= 50; p -= ANIM_SPEED_FS) { focused_window->anim_scale = p; wm_refresh(); }
    focused_window->is_fullscreen = !focused_window->is_fullscreen;
    for(int p = 50; p <= 100; p += ANIM_SPEED_FS) { focused_window->anim_scale = p; wm_refresh(); }
    focused_window->anim_scale = 100; wm_refresh();
}

void wm_switch_workspace(int dir) {
    int sw = get_screen_width();
    for (int i = 1; i <= ANIM_STEPS_WS; i++) { global_anim_x = -dir * (i * (sw / ANIM_STEPS_WS)); wm_refresh(); }
    
    current_workspace += dir;
    if (current_workspace < 0) current_workspace = 3; 
    if (current_workspace > 3) current_workspace = 0;

    focused_window = 0; Window *curr = head;
    while(curr) { if (curr->workspace == current_workspace) { focused_window = curr; break; } curr = curr->next; }
    keyboard_flush();
    
    for (int i = ANIM_STEPS_WS; i >= 0; i--) { global_anim_x = dir * (i * (sw / ANIM_STEPS_WS)); wm_refresh(); }
    global_anim_x = 0; wm_refresh();
}

void wm_swap_window(int dir) {
    if (!focused_window || window_count <= 1) return;
    Window* vis[32]; int count = 0; Window *curr = head;
    while(curr) { if (curr->workspace == current_workspace) vis[count++] = curr; curr = curr->next; }
    if (count <= 1) return;

    int idx = -1;
    for(int i=0; i<count; i++) if (vis[i] == focused_window) idx = i;
    if (idx == -1) return;
    
    int swap_idx = idx + dir;
    if (swap_idx < 0) swap_idx = count - 1;
    if (swap_idx >= count) swap_idx = 0;
    
    Window *tmp = vis[idx]; vis[idx] = vis[swap_idx]; vis[swap_idx] = tmp;
    
    Window *n_head = 0, *n_tail = 0;
    for(int i=0; i<count; i++) {
        if (!n_head) { n_head = vis[i]; n_tail = vis[i]; }
        else { n_tail->next = vis[i]; n_tail = vis[i]; }
    }
    
    curr = head;
    while(curr) {
        if (curr->workspace != current_workspace) {
            if (!n_head) { n_head = curr; n_tail = curr; }
            else { n_tail->next = curr; n_tail = curr; }
        }
        curr = curr->next;
    }
    n_tail->next = 0; head = n_head;
    wm_refresh();
}

void wm_refresh() {
    draw_rect_filled(0, TASKBAR_HEIGHT, get_screen_width(), get_screen_height() - TASKBAR_HEIGHT, DESKTOP_BG);

    Window* vis[32]; int count = 0; Window *curr = head; Window *fs_win = 0;
    while(curr) {
        if (curr->workspace == current_workspace) { vis[count++] = curr; if(curr->is_fullscreen) fs_win = curr; }
        curr = curr->next;
    }
    if (count == 0) { vesa_render_buffer(); return; }

    if (fs_win) {
        fs_win->x = wm_gaps; fs_win->y = TASKBAR_HEIGHT + wm_gaps;
        fs_win->width = get_screen_width() - wm_gaps * 2; fs_win->height = get_screen_height() - TASKBAR_HEIGHT - wm_gaps * 2;
        fs_win->cols = fs_win->width / 8; fs_win->rows = fs_win->height / 8;
        
        int r_x = fs_win->x + global_anim_x, r_y = fs_win->y;
        int r_w = fs_win->width; int r_h = fs_win->height;
        
        if (fs_win->anim_scale < 100) {
            r_w = (fs_win->width * fs_win->anim_scale) / 100;
            r_h = (fs_win->height * fs_win->anim_scale) / 100;
            r_x += (fs_win->width - r_w) / 2; r_y += (fs_win->height - r_h) / 2;
        }

        draw_rounded_rect_filled(r_x - 2, r_y - 2, r_w + 4, r_h + 4, WINDOW_RADIUS + 2, window_active_border_color);
        draw_rounded_rect_filled(r_x, r_y, r_w, r_h, WINDOW_RADIUS, fs_win->bg_color);
        if (global_anim_x == 0 && fs_win->anim_scale == 100) window_redraw_content(fs_win);
        vesa_render_rect(0, TASKBAR_HEIGHT, get_screen_width(), get_screen_height() - TASKBAR_HEIGHT); return;
    }

    int cols = count < max_grid_cols ? count : max_grid_cols;
    if (cols < 1) cols = 1;
    int rows = (count + cols - 1) / cols;
    int avail_h = get_screen_height() - TASKBAR_HEIGHT - (wm_gaps * (rows + 1));
    int total_stretch_y = 0;
    for(int r = 0; r < rows; r++) total_stretch_y += vis[r * cols]->stretch_y;

    int current_y = TASKBAR_HEIGHT + wm_gaps;
    for(int r = 0; r < rows; r++) {
        int row_items = (r == rows - 1) ? (count - r * cols) : cols;
        int row_h = (avail_h * vis[r * cols]->stretch_y) / total_stretch_y;
        int avail_w = get_screen_width() - (wm_gaps * (row_items + 1));
        int total_stretch_x = 0;
        for(int c = 0; c < row_items; c++) total_stretch_x += vis[r * cols + c]->stretch_x;

        int current_x = wm_gaps;
        for(int c = 0; c < row_items; c++) {
            Window *win = vis[r * cols + c];
            int win_w = (avail_w * win->stretch_x) / total_stretch_x;
            
            win->x = current_x; win->y = current_y;
            win->width = win_w; win->height = row_h;
            win->cols = win->width / 8; win->rows = win->height / 8;

            int r_x = win->x + global_anim_x; int r_y = win->y;
            int r_w = win->width; int r_h = win->height;

            if (win->anim_scale < 100) {
                r_w = (win->width * win->anim_scale) / 100;
                r_h = (win->height * win->anim_scale) / 100;
                r_x += (win->width - r_w) / 2; r_y += (win->height - r_h) / 2;
            }

            uint32_t f_col = (win == focused_window) ? window_active_border_color : window_border_color;
            draw_rounded_rect_filled(r_x - 2, r_y - 2, r_w + 4, r_h + 4, WINDOW_RADIUS + 2, f_col);
            draw_rounded_rect_filled(r_x, r_y, r_w, r_h, WINDOW_RADIUS, win->bg_color);
            
            if (win->anim_scale == 100 && global_anim_x == 0) window_redraw_content(win);

            current_x += win_w + wm_gaps;
        }
        current_y += row_h + wm_gaps;
    }
    vesa_render_rect(0, TASKBAR_HEIGHT, get_screen_width(), get_screen_height() - TASKBAR_HEIGHT);
}

Window* wm_create_window(uint32_t bg_color) {
    Window *win = (Window*)kmalloc(sizeof(Window));
    win->id = next_id++; win->bg_color = bg_color; win->text_color = 0xFFFFFF;
    win->cursor_x = 0; win->cursor_y = 0; win->workspace = current_workspace; 
    win->is_fullscreen = 0; win->stretch_x = 100; win->stretch_y = 100; win->anim_scale = 20;
    
    int total_chars = (get_screen_width() / 8) * (get_screen_height() / 8);
    win->char_buffer = (unsigned int*)kmalloc(total_chars * sizeof(unsigned int));
    win->color_buffer = (uint32_t*)kmalloc(total_chars * 4);
    for(int i = 0; i < total_chars; i++) { win->char_buffer[i] = ' '; win->color_buffer[i] = bg_color; }

    win->next = head; head = win; window_count++;
    focused_window = win; 
    wm_animate_open(win); 
    return win;
}

void wm_close_window(Window *win) {
    if (!win || !head) return;
    if (win->workspace == current_workspace) wm_animate_close(win); 

    if (win == head) head = head->next;
    else {
        Window *prev = head;
        while (prev->next && prev->next != win) prev = prev->next;
        if (prev->next == win) prev->next = win->next;
    }
    if(win->char_buffer) kfree(win->char_buffer);
    if(win->color_buffer) kfree(win->color_buffer);
    kfree(win); window_count--;
    
    if (focused_window == win) {
        focused_window = head; 
        if (head) set_current_output_window(head);
        else set_current_output_window(0);
    }
    wm_refresh();
}

void wm_switch_focus() {
    if (!head || window_count <= 1) return;
    if (!focused_window || !focused_window->next) focused_window = head;
    else focused_window = focused_window->next;
    keyboard_flush(); wm_set_focused_window(focused_window); wm_refresh();
}