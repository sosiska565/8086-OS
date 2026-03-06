#include "drivers/video/graphics.h"
#include "drivers/video/vesa.h"
#include "drivers/video/bga/font.h"
#include "drivers/vga/vga.h"
#include "mm/memory.h"
#include "task/task.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "drivers/mouse/mouse.h"
#include "global.h"

#define WINDOW_RADIUS 10

static int abs(int i) { return i < 0 ? -i : i; }

Window *current_output_window = 0; Window *head = 0;
int window_count = 0; int next_id = 1;
Window *focused_window = 0;
static uint32_t *blur_tmp_buffer = 0; 

int is_window_visible(Window *win) {
    if (!win) return 1;
    return win->workspace == current_workspace;
}

void wm_set_focused_window(Window *win) { focused_window = win; }

void set_current_output_window(Window *win) {
    if (current_task) current_task->window = win;
    current_output_window = win; focused_window = win;
}

void draw_rect_filled(int x, int y, int w, int h, uint32_t color) {
    extern int screen_bpp;
    if (x < 0) { w += x; x = 0; } if (y < 0) { h += y; y = 0; }
    if (x + w > get_screen_width()) w = get_screen_width() - x;
    if (y + h > get_screen_height()) h = get_screen_height() - y;
    if (w <= 0 || h <= 0) return;

    uint8_t bpp = screen_bpp / 8;
    uint8_t* base = back_buffer ? (uint8_t*)back_buffer : (uint8_t*)video_memory;

    for (int i = 0; i < h; i++) {
        uint8_t* ptr = base + ((y + i) * screen_pitch) + (x * bpp);
        if (bpp == 4) fast_memset(ptr, color, w); 
        else for (int j=0; j<w; j++) { ptr[0]=color; ptr[1]=color>>8; ptr[2]=color>>16; ptr+=bpp; }
    }
}

static const int margin_cache_r10[10] = { 10, 7, 5, 4, 3, 2, 2, 1, 1, 0 };

static inline int get_circle_margin(int r, int y) {
    if (r == 10 && y >= 0 && y < 10) return margin_cache_r10[y];
    
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
        if (i < r) { int m = get_circle_margin(r, r - i - 1); start_x += m; row_w -= m * 2; }
        else if (i >= h - r) { int m = get_circle_margin(r, i - (h - r)); start_x += m; row_w -= m * 2; }
        draw_rect_filled(start_x, y + i, row_w, 1, color);
    }
}

void draw_rounded_rect_b(int x, int y, int w, int h, int r, int thickness, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    if (r > w/2) r = w/2; if (r > h/2) r = h/2;
    if (r <= 0) {
        draw_rect_filled(x, y, w, thickness, color);
        draw_rect_filled(x, y + h - thickness, w, thickness, color);
        draw_rect_filled(x, y + thickness, thickness, h - 2*thickness, color);
        draw_rect_filled(x + w - thickness, y + thickness, thickness, h - 2*thickness, color);
        return;
    }
    for (int i = 0; i < h; i++) {
        int start_x = x; int row_w = w;
        if (i < r) { int m = get_circle_margin(r, r - i - 1); start_x += m; row_w -= m * 2; }
        else if (i >= h - r) { int m = get_circle_margin(r, i - (h - r)); start_x += m; row_w -= m * 2; }
        
        if (i < thickness || i >= h - thickness) {
            draw_rect_filled(start_x, y + i, row_w, 1, color);
        } else {
            draw_rect_filled(start_x, y + i, thickness, 1, color);
            draw_rect_filled(start_x + row_w - thickness, y + i, thickness, 1, color);
        }
    }
}

void apply_tint_only(int x, int y, int w, int h, uint32_t tint) {
    
}

void apply_blur(int x, int y, int w, int h, uint32_t tint) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > get_screen_width()) w = get_screen_width() - x;
    if (y + h > get_screen_height()) h = get_screen_height() - y;
    
    if (w <= 0 || h <= 0 || !blur_tmp_buffer) return;

    int bpp = screen_bpp / 8;
    uint8_t *buf = (uint8_t*)back_buffer;
    int radius = 8; // Фиксированный радиус для стабильности
    int div = radius * 2 + 1;
    uint32_t inv_div = (1 << 16) / div; 

    // Проход 1: Горизонтальное размытие с защитой краев
    for (int j = 0; j < h; j++) {
        int sum_r = 0, sum_g = 0, sum_b = 0;
        uint8_t *row_src = buf + ((y + j) * screen_pitch);

        for (int i = -radius; i <= radius; i++) {
            // Clamping по горизонтали: px не выйдет за [0, w-1]
            int px = (i < 0) ? 0 : (i >= w ? w - 1 : i);
            uint8_t *src = row_src + (x + px) * bpp;
            sum_b += src[0]; sum_g += src[1]; sum_r += src[2];
        }

        for (int i = 0; i < w; i++) {
            blur_tmp_buffer[j * w + i] = (((sum_r * inv_div) >> 16) << 16) | 
                                         (((sum_g * inv_div) >> 16) << 8) | 
                                          ((sum_b * inv_div) >> 16);

            int left_px = (i - radius < 0) ? 0 : i - radius;
            int right_px = (i + radius + 1 >= w) ? w - 1 : i + radius + 1;
            
            uint8_t *l_src = row_src + (x + left_px) * bpp;
            uint8_t *r_src = row_src + (x + right_px) * bpp;

            sum_r += (int)r_src[2] - (int)l_src[2];
            sum_g += (int)r_src[1] - (int)l_src[1];
            sum_b += (int)r_src[0] - (int)l_src[0];
        }
    }

    // Проход 2: Вертикальное размытие (исправляет зазор в шапке)
    for (int i = 0; i < w; i++) {
        int sum_r = 0, sum_g = 0, sum_b = 0;

        for (int j = -radius; j <= radius; j++) {
            // Clamping по вертикали: py не станет отрицательным!
            int py = (j < 0) ? 0 : (j >= h ? h - 1 : j);
            uint32_t col = blur_tmp_buffer[py * w + i];
            sum_r += (col >> 16) & 0xFF; sum_g += (col >> 8) & 0xFF; sum_b += col & 0xFF;
        }

        for (int j = 0; j < h; j++) {
            int r = (sum_r * inv_div) >> 16;
            int g = (sum_g * inv_div) >> 16;
            int b = (sum_b * inv_div) >> 16;

            // Накладываем тинт (цвет окна)
            r = (r * 3 + ((tint >> 16) & 0xFF)) >> 2;
            g = (g * 3 + ((tint >> 8) & 0xFF)) >> 2;
            b = (b * 3 + (tint & 0xFF)) >> 2;

            uint8_t *dst = buf + ((y + j) * screen_pitch) + ((x + i) * bpp);
            dst[0] = b; dst[1] = g; dst[2] = r;

            int top_py = (j - radius < 0) ? 0 : j - radius;
            int bot_py = (j + radius + 1 >= h) ? h - 1 : j + radius + 1;

            uint32_t t_col = blur_tmp_buffer[top_py * w + i];
            uint32_t b_col = blur_tmp_buffer[bot_py * w + i];

            sum_r += (int)((b_col >> 16) & 0xFF) - (int)((t_col >> 16) & 0xFF);
            sum_g += (int)((b_col >> 8) & 0xFF) - (int)((t_col >> 8) & 0xFF);
            sum_b += (int)(b_col & 0xFF) - (int)(t_col & 0xFF);
        }
    }
}

void window_restore_bg(Window *win, int local_x, int local_y, int w, int h) {
    if (!is_window_visible(win)) return;
    if (!win->cached_bg || !win->blur) {
        window_draw_rect_filled(win, local_x, local_y, w, h, win->bg_color);
        return;
    }
    int bpp = screen_bpp / 8;
    for (int cy = 0; cy < h; cy++) {
        int sy = local_y + cy;
        if (sy >= win->height || sy < 0) continue;
        
        uint32_t *src_row = win->cached_bg + (sy * win->cached_bg_w);
        uint8_t *dst = (uint8_t*)back_buffer + ((win->y + sy) * screen_pitch) + ((win->x + local_x) * bpp);
        
        int copy_w = w;
        if (local_x + copy_w > win->width) copy_w = win->width - local_x;

        if (copy_w > 0) {
            if (bpp == 4) {
                fast_memcpy(dst, src_row + local_x, copy_w * 4);
            } else {
                for (int cx = 0; cx < copy_w; cx++) {
                    uint32_t col = src_row[local_x + cx];
                    dst[cx*3] = col & 0xFF; dst[cx*3+1] = (col>>8)&0xFF; dst[cx*3+2] = (col>>16)&0xFF;
                }
            }
        }
    }
}

void window_draw_rect_filled(Window *win, int local_x, int local_y, int w, int h, uint32_t color) {
    if (!win || win->workspace != current_workspace) return; 
    if (local_x < 0) { w += local_x; local_x = 0; }
    if (local_y < 0) { h += local_y; local_y = 0; }
    if (local_x + w > win->width) w = win->width - local_x;
    if (local_y + h > win->height) h = win->height - local_y;
    if (w <= 0 || h <= 0) return;

    int abs_x = win->x + local_x, abs_y = win->y + local_y;
    int r = WINDOW_RADIUS;

    for (int i = 0; i < h; i++) {
        int draw_x = abs_x, draw_w = w, current_ly = local_y + i, margin = 0;
        if (current_ly < r) margin = get_circle_margin(r, r - current_ly - 1);
        else if (current_ly >= win->height - r) margin = get_circle_margin(r, current_ly - (win->height - r));

        if (margin > 0) {
            if (local_x < margin) { int diff = margin - local_x; draw_x += diff; draw_w -= diff; }
            int local_end = local_x + w, max_end = win->width - margin;
            if (local_end > max_end) draw_w -= (local_end - max_end);
        }
        if (draw_w > 0) draw_rect_filled(draw_x, abs_y + i, draw_w, 1, color);
    }
}

void window_draw_char(Window *win, int local_x, int local_y, unsigned int c, uint32_t color) {
    if (!win || win->workspace != current_workspace) return;
    
    uint8_t *glyph = (uint8_t*)font8x8_basic[c];
    uint8_t bpp = screen_bpp / 8; 
    uint8_t* buffer = (uint8_t*)back_buffer;
    int r = WINDOW_RADIUS;

    for (int row = 0; row < 8; row++) {
        uint8_t font_row = glyph[row];
        for (int sr = 0; sr < font_scale; sr++) { 
            int current_ly = local_y + (row * font_scale) + sr;
            if (current_ly < 0 || current_ly >= win->height) continue;

            int margin = 0;
            if (current_ly < r) margin = get_circle_margin(r, r - current_ly - 1);
            else if (current_ly >= win->height - r) margin = get_circle_margin(r, current_ly - (win->height - r));

            for (int col = 0; col < 8; col++) {
                if ((font_row >> col) & 1) {
                    for(int sc = 0; sc < font_scale; sc++) { 
                        int current_lx = local_x + (col * font_scale) + sc;
                        if (current_lx >= margin && current_lx < win->width - margin) {
                            uint8_t* dest = buffer + ((win->y + current_ly) * screen_pitch) + ((win->x + current_lx) * bpp);
                            
                            if (bpp == 4) {
                                *(uint32_t*)dest = color;
                            } else {
                                dest[0] = color & 0xFF;
                                dest[1] = (color >> 8) & 0xFF;
                                dest[2] = (color >> 16) & 0xFF;
                            }
                        }
                    }
                }
            }
        }
    }
}

void window_redraw_char_at(Window *win, int col, int row) {
    if (!is_window_visible(win)) return;
    int fw = 8 * font_scale;
    int lx = col * fw;
    int ly = row * fw;
    window_restore_bg(win, lx, ly, fw, fw);
    unsigned int symb = win->char_buffer[row * win->cols + col];
    uint32_t color = win->color_buffer[row * win->cols + col];
    if (symb != 0 && symb != ' ') window_draw_char(win, lx, ly, symb, color);
    vesa_render_rect(win->x + lx, win->y + ly, fw, fw);
}

void buffer_write(Window *win, int col, int row, unsigned int c, uint32_t color) {
    int max_cols = win->cols;
    if (!win->char_buffer) return;
    win->char_buffer[row * max_cols + col] = c;
    win->color_buffer[row * max_cols + col] = color;
}

void window_redraw_content(Window *win) {
    if (!win || !win->char_buffer || win->workspace != current_workspace) return;
    int fw = 8 * font_scale;
    for (int r = 0; r < win->rows; r++) {
        for (int c = 0; c < win->cols; c++) {
            unsigned int symb = win->char_buffer[r * win->cols + c];
            uint32_t color = win->color_buffer[r * win->cols + c];
            if (symb != 0 && symb != ' ') window_draw_char(win, c * fw, r * fw, symb, color);
        }
    }
}

void window_clear(Window *win, uint32_t color){
    if (win->char_buffer) {
        for(int i = 0; i < win->rows * win->cols; i++) { win->char_buffer[i] = ' '; win->color_buffer[i] = color; }
    }
    win->cursor_x = 0; win->cursor_y = 0;
    if (is_window_visible(win)) {
        if (win->blur) wm_refresh();
        else {
            draw_rounded_rect_filled(win->x, win->y, win->width, win->height, WINDOW_RADIUS, color);
            wm_render_window(win);
        }
    }
}

void window_scroll(Window *win){
    if (win->char_buffer) {
        for (int r = 0; r < win->rows - 1; r++) {
            for (int c = 0; c < win->cols; c++) {
                int cur = r * win->cols + c, nxt = (r + 1) * win->cols + c;
                win->char_buffer[cur] = win->char_buffer[nxt]; win->color_buffer[cur] = win->color_buffer[nxt];
            }
        }
        int l_start = (win->rows - 1) * win->cols;
        for (int c = 0; c < win->cols; c++) { win->char_buffer[l_start + c] = ' '; win->color_buffer[l_start + c] = win->bg_color; }
    }
    win->cursor_x = 0; win->cursor_y = (win->rows - 1) * (8 * font_scale);
    
    if (is_window_visible(win)) {
        if (win->blur) {
            window_restore_bg(win, 0, 0, win->width, win->height);
            window_redraw_content(win);
            vesa_render_rect(win->x, win->y, win->width, win->height);
        } else {
            draw_rounded_rect_filled(win->x, win->y, win->width, win->height, WINDOW_RADIUS, win->bg_color);
            window_redraw_content(win);
        }
    }
}

void window_print(Window *win, int x, int y, char *str, uint32_t color) {
    int old_x = win->cursor_x; int old_y = win->cursor_y; uint32_t old_c = win->text_color;
    win->cursor_x = x; win->cursor_y = y; win->text_color = color;
    while (*str) { window_putc(win, *str); str++; }
    win->cursor_x = old_x; win->cursor_y = old_y; win->text_color = old_c;
}

void window_render_char_rect(Window *win, int col, int row) {
    if (!is_window_visible(win)) return;
    int fw = 8 * font_scale;
    vesa_render_rect(win->x + col * fw, win->y + row * fw, fw, fw);
}

void window_putc(Window *win, unsigned int c) {
    if (win == 0) return;
    int fw = 8 * font_scale;
    int col_idx = win->cursor_x / fw; int row_idx = win->cursor_y / fw;

    if (c == '\n') { win->cursor_x = 0; win->cursor_y += fw; } 
    else if (c == '\b') {
        if (win->cursor_x >= fw) win->cursor_x -= fw;
        else if (win->cursor_y >= fw) { win->cursor_x = (win->cols - 1) * fw; win->cursor_y -= fw; }
        else return;
        
        col_idx = win->cursor_x / fw; row_idx = win->cursor_y / fw;
        buffer_write(win, col_idx, row_idx, ' ', win->bg_color);
        
        if (is_window_visible(win)) { 
            if (win->blur) window_restore_bg(win, win->cursor_x, win->cursor_y, fw, fw);
            else window_draw_rect_filled(win, win->cursor_x, win->cursor_y, fw, fw, win->bg_color);
            window_render_char_rect(win, col_idx, row_idx);
        }
    } else {
        buffer_write(win, col_idx, row_idx, c, win->text_color);
        if (is_window_visible(win)) {
            window_draw_char(win, win->cursor_x, win->cursor_y, c, win->text_color);
            window_render_char_rect(win, col_idx, row_idx);
        }
        win->cursor_x += fw;
    }

    if (win->cursor_x >= win->width - fw) { win->cursor_x = 0; win->cursor_y += fw; }
    if (win->cursor_y >= win->height - fw) {
        window_scroll(win);
        if (is_window_visible(win) && !win->blur) vesa_render_rect(win->x, win->y, win->width, win->height);
    }
}

int old_mx = -1, old_my = -1;
void wm_update_cursor() {
    int bpp = screen_bpp / 8;
    if(old_mx >= 0) { 
        for (int i=0; i<20; i++) {
            if(old_my+i >= get_screen_height()) break;
            uint8_t *s = (uint8_t*)back_buffer + (old_my+i)*screen_pitch + old_mx*bpp;
            uint8_t *d = (uint8_t*)video_memory + (old_my+i)*screen_pitch + old_mx*bpp;
            for(int j=0; j<15; j++) {
                if(old_mx+j < get_screen_width()) {
                    if (bpp == 4) {
                        ((uint32_t*)d)[j] = ((uint32_t*)s)[j];
                    } else {
                        d[j*3] = s[j*3];
                        d[j*3+1] = s[j*3+1];
                        d[j*3+2] = s[j*3+2];
                    }
                }
            }
        }
    }
    old_mx = mouse.x; old_my = mouse.y;
    for(int i=0; i<15; i++) { 
        for(int j=0; j<=i/2; j++) {
            if (old_mx+j < get_screen_width() && old_my+i < get_screen_height()) {
                uint8_t *d = (uint8_t*)video_memory + (old_my+i)*screen_pitch + (old_mx+j)*bpp;
                if (bpp == 4) {
                    *(uint32_t*)d = 0xFFFFFFFF;
                } else {
                    d[0] = 0xFF; d[1] = 0xFF; d[2] = 0xFF;
                }
            }
        }
    }
}

void wm_init(){ 
    head = 0; window_count = 0; 
    if (!blur_tmp_buffer) {
        blur_tmp_buffer = kmalloc_a(get_screen_width() * get_screen_height() * sizeof(uint32_t));
    }
}

void wm_render_window(Window *win) {
    if (!is_window_visible(win)) return;
    vesa_render_rect(win->x - 5, win->y - 5, win->width + 10, win->height + 10);
}

void wm_animate_open(Window *win) {
    win->anim_scale = 100; 
    wm_refresh();
}

void wm_animate_close(Window *win) {
    win->anim_scale = 0; 
}

void wm_toggle_fullscreen() {
    if (!focused_window) return;
    focused_window->is_fullscreen = !focused_window->is_fullscreen;
    focused_window->anim_scale = 100; 
    wm_refresh();
}

void wm_switch_workspace(int dir) {
    current_workspace += dir;
    if (current_workspace < 0) current_workspace = 3; 
    if (current_workspace > 3) current_workspace = 0;
    
    focused_window = 0; 
    Window *curr = head;
    while(curr) { 
        if (curr->workspace == current_workspace) { focused_window = curr; break; } 
        curr = curr->next; 
    }
    wm_refresh();
}

void wm_swap_window(int dir) {
    if (!focused_window || window_count <= 1) return;
    Window* vis[32]; int count = 0; Window *curr = head;
    while(curr) { if (curr->workspace == current_workspace) vis[count++] = curr; curr = curr->next; }
    if (count <= 1) return;

    int idx = -1; for(int i=0; i<count; i++) if (vis[i] == focused_window) idx = i;
    if (idx == -1) return;
    
    int swap_idx = idx + dir;
    if (swap_idx < 0) swap_idx = count - 1; if (swap_idx >= count) swap_idx = 0;
    
    Window *tmp = vis[idx]; vis[idx] = vis[swap_idx]; vis[swap_idx] = tmp;
    
    Window *n_head = 0, *n_tail = 0;
    for(int i=0; i<count; i++) {
        if (!n_head) { n_head = vis[i]; n_tail = vis[i]; } else { n_tail->next = vis[i]; n_tail = vis[i]; }
    }
    curr = head;
    while(curr) {
        if (curr->workspace != current_workspace) {
            if (!n_head) { n_head = curr; n_tail = curr; } else { n_tail->next = curr; n_tail = curr; }
        }
        curr = curr->next;
    }
    n_tail->next = 0; head = n_head; wm_refresh();
}

void wm_refresh() {
    int bpp_bytes = screen_bpp / 8;
    if (wallpaper_buf) {
        uint8_t *dst = (uint8_t*)back_buffer + (TASKBAR_HEIGHT * screen_pitch);
        uint8_t *src = (uint8_t*)wallpaper_buf + (TASKBAR_HEIGHT * screen_pitch);
        int size = (get_screen_height() - TASKBAR_HEIGHT) * screen_pitch;
        fast_memcpy(dst, src, size);
    } else {
        draw_rect_filled(0, TASKBAR_HEIGHT, get_screen_width(), get_screen_height() - TASKBAR_HEIGHT, DESKTOP_BG);
    }

    Window* vis[32]; int count = 0; Window *curr = head; Window *fs_win = 0;
    while(curr) {
        if (curr->workspace == current_workspace) { vis[count++] = curr; if(curr->is_fullscreen) fs_win = curr; }
        curr = curr->next;
    }
    
    if (count > 0) {
        if (fs_win) {
            fs_win->x = wm_gaps; fs_win->y = TASKBAR_HEIGHT + wm_gaps;
            fs_win->width = get_screen_width() - wm_gaps * 2; fs_win->height = get_screen_height() - TASKBAR_HEIGHT - wm_gaps * 2;
            fs_win->cols = fs_win->width / (8 * font_scale); fs_win->rows = fs_win->height / (8 * font_scale);
            
            int r_x = fs_win->x, r_y = fs_win->y;
            int r_w = fs_win->width, r_h = fs_win->height;
            
            if (fs_win->blur) apply_blur(r_x, r_y, r_w, r_h, fs_win->bg_color);

            draw_rounded_rect_b(r_x - 2, r_y - 2, r_w + 4, r_h + 4, WINDOW_RADIUS + 2, 2, window_active_border_color);
            if (!fs_win->blur) draw_rounded_rect_filled(r_x, r_y, r_w, r_h, WINDOW_RADIUS, fs_win->bg_color);
            
            if (fs_win->cached_bg_w != r_w || fs_win->cached_bg_h != r_h) {
                if (fs_win->cached_bg) kfree(fs_win->cached_bg);
                fs_win->cached_bg = kmalloc(r_w * r_h * 4);
                fs_win->cached_bg_w = r_w; fs_win->cached_bg_h = r_h;
            }
            if (fs_win->cached_bg) {
                for (int cy = 0; cy < r_h; cy++) {
                    uint8_t *src = (uint8_t*)back_buffer + ((r_y + cy) * screen_pitch) + r_x * bpp_bytes;
                    uint32_t *dst = fs_win->cached_bg + cy * r_w;
                    for(int cx=0; cx<r_w; cx++) {
                        if (bpp_bytes == 4) dst[cx] = ((uint32_t*)src)[cx];
                        else dst[cx] = src[cx*3] | (src[cx*3+1]<<8) | (src[cx*3+2]<<16);
                    }
                }
            }

            window_redraw_content(fs_win);

        } else {
            int cols = count < max_grid_cols ? count : max_grid_cols; if (cols < 1) cols = 1;
            int rows = (count + cols - 1) / cols;
            int avail_h = get_screen_height() - TASKBAR_HEIGHT - (wm_gaps * (rows + 1));
            int total_stretch_y = 0; for(int r = 0; r < rows; r++) total_stretch_y += vis[r * cols]->stretch_y;

            int current_y = TASKBAR_HEIGHT + wm_gaps;
            for(int r = 0; r < rows; r++) {
                int row_items = (r == rows - 1) ? (count - r * cols) : cols;
                int row_h = (avail_h * vis[r * cols]->stretch_y) / total_stretch_y;
                int avail_w = get_screen_width() - (wm_gaps * (row_items + 1));
                int total_stretch_x = 0; for(int c = 0; c < row_items; c++) total_stretch_x += vis[r * cols + c]->stretch_x;

                int current_x = wm_gaps;
                for(int c = 0; c < row_items; c++) {
                    Window *win = vis[r * cols + c];
                    int win_w = (avail_w * win->stretch_x) / total_stretch_x;
                    
                    win->x = current_x; win->y = current_y;
                    win->width = win_w; win->height = row_h;
                    win->cols = win->width / (8 * font_scale); win->rows = win->height / (8 * font_scale);

                    int r_x = win->x, r_y = win->y;
                    int r_w = win->width, r_h = win->height;
                    uint32_t f_col = (win == focused_window) ? window_active_border_color : window_border_color;

                    if (win->blur) apply_blur(r_x, r_y, r_w, r_h, win->bg_color);

                    draw_rounded_rect_b(r_x - 2, r_y - 2, r_w + 4, r_h + 4, WINDOW_RADIUS + 2, 2, f_col);
                    if (!win->blur) draw_rounded_rect_filled(r_x, r_y, r_w, r_h, WINDOW_RADIUS, win->bg_color);
                    
                    if (win->cached_bg_w != r_w || win->cached_bg_h != r_h) {
                        if (win->cached_bg) kfree(win->cached_bg);
                        win->cached_bg = kmalloc(r_w * r_h * 4);
                        win->cached_bg_w = r_w; win->cached_bg_h = r_h;
                    }
                    if (win->cached_bg) {
                        for (int cy = 0; cy < r_h; cy++) {
                            uint8_t *src = (uint8_t*)back_buffer + ((r_y + cy) * screen_pitch) + r_x * bpp_bytes;
                            uint32_t *dst = win->cached_bg + cy * r_w;
                            for(int cx=0; cx<r_w; cx++) {
                                if (bpp_bytes == 4) dst[cx] = ((uint32_t*)src)[cx];
                                else dst[cx] = src[cx*3] | (src[cx*3+1]<<8) | (src[cx*3+2]<<16);
                            }
                        }
                    }

                    window_redraw_content(win);
                    
                    current_x += win_w + wm_gaps;
                }
                current_y += row_h + wm_gaps;
            }
        }
    }
    vesa_render_buffer(); 
    wm_update_cursor();   
}

Window* wm_create_window(uint32_t bg_color) {
    Window *win = (Window*)kmalloc(sizeof(Window));
    if (!win) return 0; 
    
    win->id = next_id++; 
    win->blur = (bg_color & 0x80000000) ? 1 : 0;
    win->bg_color = bg_color & 0x00FFFFFF; 
    win->text_color = 0xFFFFFF;
    win->cursor_x = 0; win->cursor_y = 0; win->workspace = current_workspace; 
    win->is_fullscreen = 0; win->stretch_x = 100; win->stretch_y = 100; 
    win->anim_scale = 100; 
    win->cached_bg = 0; win->cached_bg_w = 0; win->cached_bg_h = 0;
    
    int total_chars = (get_screen_width() / 8) * (get_screen_height() / 8);
    win->char_buffer = (unsigned int*)kmalloc(total_chars * sizeof(unsigned int));
    win->color_buffer = (uint32_t*)kmalloc(total_chars * 4);
    
    if (!win->char_buffer || !win->color_buffer) {
        if(win->char_buffer) kfree(win->char_buffer);
        if(win->color_buffer) kfree(win->color_buffer);
        kfree(win);
        return 0;
    }
    
    for(int i = 0; i < total_chars; i++) { win->char_buffer[i] = ' '; win->color_buffer[i] = win->bg_color; }

    win->next = head; head = win; window_count++;
    focused_window = win; 
    wm_animate_open(win); 
    return win;
}

void wm_close_window(Window *win) {
    if (!win || !head) return;
    if (win->workspace == current_workspace) wm_animate_close(win); 
    if (win == head) head = head->next;
    else { Window *p = head; while(p->next && p->next != win) p=p->next; if(p->next == win) p->next = win->next; }
    if (win->char_buffer) kfree(win->char_buffer); 
    if (win->color_buffer) kfree(win->color_buffer);
    if(win->cached_bg) kfree(win->cached_bg); 
    kfree(win); window_count--;
    if (focused_window == win) { focused_window = head; if (head) set_current_output_window(head); else set_current_output_window(0); }
    wm_refresh();
}

void wm_switch_focus() {
    if (!head || window_count <= 1) return;
    if (!focused_window || !focused_window->next) focused_window = head; else focused_window = focused_window->next;
    keyboard_flush(); wm_set_focused_window(focused_window); wm_refresh();
}