#include "drivers/video/graphics.h"
#include "drivers/video/vesa.h"
#include "drivers/video/bga/font.h"
#include "drivers/vga/vga.h"
#include "memory/memory.h"
#include "multitask/task.h"
#include "drivers/keyboard/keyboardDriver.h"

#define FONT_W 8
#define FONT_H 8

static int abs(int i) { return i < 0 ? -i : i; }
static void swap(int* a, int* b) { int t = *a; *a = *b; *b = t; }

Window *current_output_window = 0;
Window *head = 0;
int window_count = 0;
int next_id = 1;
Window *focused_window = 0;

void wm_set_focused_window(Window *win) {
    focused_window = win;
    wm_refresh();
}

void set_current_output_window(Window *win) {
    if (current_task) {
        current_task->window = win;
    }
    current_output_window = win;
    focused_window = win;
}

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
        put_pixel(x0 + x, y0 + y, color); put_pixel(x0 - x, y0 + y, color);
        put_pixel(x0 + x, y0 - y, color); put_pixel(x0 - x, y0 - y, color);
        put_pixel(x0 + y, y0 + x, color); put_pixel(x0 - y, y0 + x, color);
        put_pixel(x0 + y, y0 - x, color); put_pixel(x0 - y, y0 - x, color);
        x++;
        if (d > 0) { y--; d = d + 4 * (x - y) + 10; }
        else { d = d + 4 * x + 6; }
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
        if (d > 0) { y--; d = d + 4 * (x - y) + 10; }
        else { d = d + 4 * x + 6; }
    }
}

void buffer_write(Window *win, int col, int row, char c, uint32_t color) {
    int max_cols = get_screen_width() / FONT_W;
    
    if (!win->char_buffer || !win->color_buffer) return;
    
    int idx = row * max_cols + col;
    win->char_buffer[idx] = c;
    win->color_buffer[idx] = color;
}

void window_put_pixel(Window *win, int x, int y, uint32_t color){
    if(x < 0 || x >= win->width || y < 0 || y >= win->height) return;
    put_pixel(win->x + x, win->y + y, color);
}

void window_draw_char(Window *win, int x, int y, char c, uint32_t color) {
    if(!win) return;
    uint8_t *glyph = (uint8_t*)font8x8_basic[(int)((unsigned char)c)];

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            int bit = (glyph[row] >> col) & 1;
            if (bit) {
                window_put_pixel(win, x + col, y + row, color);
            }
        }
    }
}

void window_redraw_content(Window *win) {
    if (!win || !win->char_buffer) return;

    int max_cols = get_screen_width() / FONT_W;
    
    for (int r = 0; r < win->rows; r++) {
        for (int c = 0; c < win->cols; c++) {
            int idx = r * max_cols + c;
            char symb = win->char_buffer[idx];
            uint32_t color = win->color_buffer[idx];
            
            if (symb != 0 && symb != ' ') {
                window_draw_char(win, c * FONT_W, r * FONT_H, symb, color);
            }
        }
    }
}

void window_clear(Window *win, uint32_t color){
    draw_rect_filled(win->x, win->y, win->width, win->height, color);
    
    int max_cols = get_screen_width() / FONT_W;
    int max_rows = get_screen_height() / FONT_H;
    int total = max_cols * max_rows;
    
    if (win->char_buffer) {
        for(int i = 0; i < total; i++) {
            win->char_buffer[i] = ' ';
            win->color_buffer[i] = color;
        }
    }
    
    win->cursor_x = 0;
    win->cursor_y = 0;
}

void window_print(Window *win, int x, int y, char *str, uint32_t color) {
    int old_x = win->cursor_x;
    int old_y = win->cursor_y;
    uint32_t old_c = win->text_color;
    
    win->cursor_x = x;
    win->cursor_y = y;
    win->text_color = color;
    
    while (*str) {
        window_putc(win, *str);
        str++;
    }
    
    win->cursor_x = old_x;
    win->cursor_y = old_y;
    win->text_color = old_c;
}

void window_scroll(Window *win){
    draw_rect_filled(win->x, win->y, win->width, win->height, win->bg_color);
    
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
    
    win->cursor_x = 0;
    win->cursor_y = (win->rows - 1) * FONT_H;
    
    window_redraw_content(win);
}

void window_putc(Window *win, char c) {
    if (win == 0) return;

    int max_cols = get_screen_width() / FONT_W;
    
    int col_idx = win->cursor_x / FONT_W;
    int row_idx = win->cursor_y / FONT_H;

    if (c == '\n') {
        win->cursor_x = 0;
        win->cursor_y += 8;
    } 
    else if (c == '\b') {
        if (win->cursor_x >= 8) {
            win->cursor_x -= 8;
            col_idx = win->cursor_x / FONT_W;
            
            for(int y = 0; y < 8; y++) {
                for(int x = 0; x < 8; x++) {
                    window_put_pixel(win, win->cursor_x + x, win->cursor_y + y, win->bg_color);
                }
            }
            buffer_write(win, col_idx, row_idx, ' ', win->bg_color);
        }
    }
    else {
        window_draw_char(win, win->cursor_x, win->cursor_y, c, win->text_color);

        buffer_write(win, col_idx, row_idx, c, win->text_color);
        
        win->cursor_x += 8;
    }

    if (win->cursor_x >= win->width - 8) {
        win->cursor_x = 0;
        win->cursor_y += 8;
    }

    if (win->cursor_y >= win->height - 8) {
        window_scroll(win);
    }
}


void wm_init(){
    head = 0;
    window_count = 0;
}

void draw_window(Window *win, int x, int y, int w, int h, uint32_t color_frame, uint32_t color_bg, int centered) {
    if(centered){
    }
    draw_rect_filled(x - 3, y - 3, w + 6, h + 6, color_frame);
    draw_rect_filled(x, y, w, h, color_bg);
}

void wm_refresh(){
    clear_screen_vesa(0x000000);

    if(window_count == 0) return;

    int cols = 1;
    if (window_count > 1) cols = 2;
    if (window_count > 4) cols = 3;
    
    int rows = (window_count + cols - 1) / cols;
    if(rows < 1) rows = 1;
    
    int scr_w = get_screen_width();
    int scr_h = get_screen_height();
    
    int win_w = (scr_w - (GAP * (cols + 1))) / cols;
    int win_h = (scr_h - (GAP * (rows + 1))) / rows;

    Window *curr = head;
    int i = 0;
    
    while(curr != 0) {
        int c = i % cols;
        int r = i / cols;
        
        curr->x = GAP + c * (win_w + GAP);
        curr->y = GAP + r * (win_h + GAP);
        curr->width = win_w;
        curr->height = win_h;
        
        curr->cols = curr->width / 8;
        curr->rows = curr->height / 8;

        uint32_t frame_color = (curr == focused_window) ? 0x00FF00 : 0x555555;

        // Рисуем тело окна
        draw_rect_filled(curr->x - 2, curr->y - 2, curr->width + 4, curr->height + 4, frame_color);
        draw_rect_filled(curr->x, curr->y, curr->width, curr->height, curr->bg_color);
        
        window_redraw_content(curr);
        
        curr = curr->next;
        i++;
    }
}

Window* wm_create_window(uint32_t bg_color) {
    Window *win = (Window*)kmalloc(sizeof(Window));
    win->id = next_id++;
    win->bg_color = bg_color;
    win->text_color = 0xFFFFFF;
    win->cursor_x = 0;
    win->cursor_y = 0;
    
    int max_w = get_screen_width();
    int max_h = get_screen_height();
    int max_cols = max_w / FONT_W;
    int max_rows = max_h / FONT_H;
    int total_chars = max_cols * max_rows;

    win->char_buffer = (char*)kmalloc(total_chars);
    win->color_buffer = (uint32_t*)kmalloc(total_chars * 4);
    
    for(int i = 0; i < total_chars; i++) {
        win->char_buffer[i] = ' '; 
        win->color_buffer[i] = bg_color;
    }

    win->next = head;
    head = win;
    window_count++;
    
    focused_window = win;
    wm_refresh();
    
    return win;
}

void wm_close_window(Window *win) {
    if (win == 0 || head == 0) return;

    if (win == head) {
        head = head->next;
    } else {
        Window *prev = head;
        while (prev->next != 0 && prev->next != win) {
            prev = prev->next;
        }
        if (prev->next == win) {
            prev->next = win->next;
        } else {
            return;
        }
    }

    if(win->char_buffer) kfree(win->char_buffer);
    if(win->color_buffer) kfree(win->color_buffer);
    
    kfree(win);
    window_count--;
    
    if (focused_window == win) {
        focused_window = head; 
        if (head) set_current_output_window(head);
        else set_current_output_window(0);
    }
    
    wm_refresh();
}

void wm_switch_focus() {
    if (head == 0 || window_count <= 1) return;

    if (focused_window == 0 || focused_window->next == 0) {
        focused_window = head;
    } else {
        focused_window = focused_window->next;
    }

    keyboard_flush();
    wm_set_focused_window(focused_window);
    wm_refresh();
}