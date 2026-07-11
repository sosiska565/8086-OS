/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/lib/libgui.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include "libgui.h"

#define STBTT_assert(x) ((void)(0)) 
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define SYSTEM_FONT_PATH "/system/font/font.ttf"

stbtt_fontinfo font;
uint8_t* ttf_buffer = NULL;
int font_initialized = 0;

static WM_Queue* wm_queue = NULL;
static int shm_key_counter = 0;
float g_ui_scale = -1.0f;

float current_font_size = 16.0f;

typedef struct {
    unsigned char* bitmap;
    int bw, bh, bxoff, byoff, advance;
} GlyphCache;

GlyphCache glyph_cache[128]; 
int font_ascent = 0;

static inline uint32_t fast_blend(uint32_t fg, uint32_t bg, uint8_t alpha) {
    if (alpha == 0) return bg;
    if (alpha == 255) return fg;
    uint32_t rb = bg & 0x00FF00FF;
    uint32_t g  = bg & 0x0000FF00;
    rb += (((fg & 0x00FF00FF) - rb) * alpha) >> 8;
    g  += (((fg & 0x0000FF00) - g ) * alpha) >> 8;
    return (rb & 0x00FF00FF) | (g & 0x0000FF00);
}

void gui_set_font_size(float size) {
    if (!ttf_buffer) return;
    
    if (size < 8.0f) size = 8.0f;
    if (size > 72.0f) size = 72.0f;
    current_font_size = size;

    if (font_initialized) {
        for (int i = 32; i < 128; i++) {
            if (glyph_cache[i].bitmap) {
                stbtt_FreeBitmap(glyph_cache[i].bitmap, NULL);
                glyph_cache[i].bitmap = NULL;
            }
        }
    }

    float scale = stbtt_ScaleForPixelHeight(&font, current_font_size);
    int descent, lineGap;
    stbtt_GetFontVMetrics(&font, &font_ascent, &descent, &lineGap);
    font_ascent = (int)(font_ascent * scale);

    for (int i = 32; i < 128; i++) {
        int advance, lsb;
        stbtt_GetCodepointHMetrics(&font, i, &advance, &lsb);
        glyph_cache[i].advance = (int)(advance * scale);
        glyph_cache[i].bitmap = stbtt_GetCodepointBitmap(&font, 0, scale, i, 
            &glyph_cache[i].bw, &glyph_cache[i].bh, 
            &glyph_cache[i].bxoff, &glyph_cache[i].byoff);
    }
    font_initialized = 1;
}

void init_vector_fonts() {
    if (font_initialized) return;
    int sz = get_file_size(SYSTEM_FONT_PATH);
    if (sz > 0) {
        ttf_buffer = malloc(sz);
        read_file(SYSTEM_FONT_PATH, ttf_buffer);
        stbtt_InitFont(&font, ttf_buffer, 0);
        
        gui_set_font_size(current_font_size); 
    }
}

int font_get_width(unsigned int c) {
    if (!font_initialized || c >= 128) return 8;
    return glyph_cache[c].advance;
}

int gui_get_font_height() {
    if (!font_initialized) return 16;
    float scale = stbtt_ScaleForPixelHeight(&font, current_font_size);
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
    return (int)((ascent - descent + lineGap) * scale);
}

int gui_get_font_width() {
    return font_get_width('W'); 
}

int gui_get_string_width(const char *str, int len) {
    if (!font_initialized) return len * 8;
    int w = 0;
    for (int i = 0; i < len && str[i]; i++) {
        unsigned char c = str[i];
        if (c < 128) w += glyph_cache[c].advance;
    }
    return w;
}

void gui_draw_string(gui_window_t* win, int x, int y, const char *str, uint32_t fg) {
    if (!font_initialized) return;

    int cx = x;
    int real_w = (int)(win->w * g_ui_scale);
    int real_h = (int)(win->h * g_ui_scale);

    while (*str) {
        unsigned char c = *str++;
        if (c < 32 || c >= 128) continue; 

        GlyphCache *gc = &glyph_cache[c];
        if (gc->bitmap) {
            int out_x = cx + gc->bxoff;
            int out_y = y + font_ascent + gc->byoff; 

            for (int by = 0; by < gc->bh; by++) {
                for (int bx = 0; bx < gc->bw; bx++) {
                    uint8_t alpha = gc->bitmap[by * gc->bw + bx];
                    if (alpha > 0) {
                        int px = (int)((out_x + bx) * g_ui_scale);
                        int py = (int)((out_y + by) * g_ui_scale);
                        if (px >= 0 && px < real_w && py >= 0 && py < real_h) {
                            uint32_t bg = win->backbuffer[py * real_w + px];
                            win->backbuffer[py * real_w + px] = fast_blend(fg, bg, alpha);
                        }
                    }
                }
            }
        }
        cx += gc->advance;
    }
}

void gui_draw_char(gui_window_t* win, int x, int y, char c, uint32_t fg) {
    char str[2] = {c, '\0'};
    gui_draw_string(win, x, y, str, fg);
}

void init_ui_scale() {
    if (g_ui_scale > 0.0f) return;
    char* s = getenv("UI_SCALE");
    if (s) {
        int val = atoi(s);
        g_ui_scale = (float)val / 100.0f;
    } else {
        g_ui_scale = 1.0f;
    }
    if (g_ui_scale < 0.5f) g_ui_scale = 0.5f;
    if (g_ui_scale > 3.0f) g_ui_scale = 3.0f;
}

gui_window_t* gui_create_window(const char* title, int w, int h) {
    if (w <= 0 || h <= 0 || w > 4000 || h > 4000) return NULL;
    init_ui_scale();
    init_vector_fonts(); 
    
    if (!wm_queue) {
        shm_get(WM_SHM_KEY, sizeof(WM_Queue));
        wm_queue = (WM_Queue*)shm_map(WM_SHM_KEY);
    }

    gui_window_t* win = malloc(sizeof(gui_window_t));
    if (!win) return NULL;

    int real_w = (int)(w * g_ui_scale);
    int real_h = (int)(h * g_ui_scale);

    win->w = w;
    win->h = h;
    win->mx = 0;
    win->my = 0;
    win->mbtn = 0;
    win->closed = 0;
    win->cursor_pos = 0;

    shm_key_counter++;
    win->shm_key = get_ticks() + 10000 + (shm_key_counter * 100);

    shm_get(win->shm_key, sizeof(Client_SHM) + (real_w * real_h * 4));
    win->shared_mem = (Client_SHM*)shm_map(win->shm_key);
    
    if (!win->shared_mem) {
        shm_get(win->shm_key, 0);
        free(win);
        return NULL;
    }

    win->shared_mem->event_head = 0;
    win->shared_mem->event_tail = 0;
    win->pixels = win->shared_mem->pixels;

    win->backbuffer = malloc(real_w * real_h * 4);
    if (!win->backbuffer) {
        shm_get(win->shm_key, 0);
        free(win);
        return NULL;
    }
    memset(win->backbuffer, 0, real_w * real_h * 4);

    int tail = wm_queue->tail;
    wm_queue->commands[tail].type = WM_CMD_CREATE_WINDOW;
    wm_queue->commands[tail].shm_key = win->shm_key;
    wm_queue->commands[tail].w = real_w;
    wm_queue->commands[tail].h = real_h;
    strcpy(wm_queue->commands[tail].title, title);
    wm_queue->tail = (tail + 1) % 32;

    return win;
}

gui_window_t* gui_create_frameless(int x, int y, int w, int h) {
    if (w <= 0 || h <= 0 || w > 4000 || h > 4000) return NULL;
    init_ui_scale();
    init_vector_fonts(); 
    
    if (!wm_queue) {
        shm_get(WM_SHM_KEY, sizeof(WM_Queue));
        wm_queue = (WM_Queue*)shm_map(WM_SHM_KEY);
    }

    gui_window_t* win = malloc(sizeof(gui_window_t));
    if (!win) return NULL;

    win->w = w;
    win->h = h;
    win->mx = 0;
    win->my = 0;
    win->mbtn = 0;
    win->closed = 0;
    win->cursor_pos = 0;

    shm_key_counter++;
    win->shm_key = get_ticks() + 15000 + (shm_key_counter * 100);

    shm_get(win->shm_key, sizeof(Client_SHM) + (w * h * 4));
    win->shared_mem = (Client_SHM*)shm_map(win->shm_key);
    
    if (!win->shared_mem) {
        shm_get(win->shm_key, 0);
        free(win);
        return NULL;
    }

    win->shared_mem->event_head = 0;
    win->shared_mem->event_tail = 0;
    win->pixels = win->shared_mem->pixels;

    win->backbuffer = malloc(w * h * 4);
    if (!win->backbuffer) {
        shm_get(win->shm_key, 0);
        free(win);
        return NULL;
    }
    memset(win->backbuffer, 0, w * h * 4);

    int tail = wm_queue->tail;
    wm_queue->commands[tail].type = WM_CMD_CREATE_FRAMELESS;
    wm_queue->commands[tail].shm_key = win->shm_key;
    wm_queue->commands[tail].x = x;
    wm_queue->commands[tail].y = y;
    wm_queue->commands[tail].w = w;
    wm_queue->commands[tail].h = h;
    wm_queue->tail = (tail + 1) % 32;

    return win;
}

int gui_poll_event(gui_window_t* win, GUI_Event* ev) {
    if (!wm_queue || (get_ticks() % 100 == 0 && shm_map(WM_SHM_KEY) == NULL)) {
        win->closed = 1;
        return 0;
    }
    if (win->shared_mem->event_head == win->shared_mem->event_tail) return 0;
    int head = win->shared_mem->event_head;
    *ev = win->shared_mem->events[head];
    win->shared_mem->event_head = (head + 1) % 32;
    return 1;
}

void gui_update(gui_window_t* win) {
    if (!wm_queue) {
        shm_get(WM_SHM_KEY, sizeof(WM_Queue));
        wm_queue = (WM_Queue*)shm_map(WM_SHM_KEY);
    }

    if (wm_queue && wm_queue->global_scale >= 0.5f) {
        float diff = wm_queue->global_scale - g_ui_scale;
        if (diff < 0) diff = -diff;
        if (diff > 0.01f) {
            g_ui_scale = wm_queue->global_scale;
            gui_resize_buffer(win, win->w, win->h);
        }
    }

    GUI_Event ev;
    win->clicked = 0;
    win->char_input = 0;
    win->key_code = 0;
    win->scroll_z = 0;

    while (gui_poll_event(win, &ev)) {
        if (ev.type == GUI_EV_CLOSE) win->closed = 1;
        else if (ev.type == GUI_EV_MOUSE_MOVE || ev.type == GUI_EV_MOUSE_CLICK) {
            win->mx = (int)(ev.x / g_ui_scale);
            win->my = (int)(ev.y / g_ui_scale);
            win->mbtn = ev.button;
            if (ev.type == GUI_EV_MOUSE_CLICK && ev.button == 1) win->clicked = 1;
        }
        else if (ev.type == GUI_EV_KEY_PRESS) {
            win->key_code = ev.keycode;
            win->char_input = (char)(ev.keycode & 0xFF);
        }
        else if (ev.type == GUI_EV_RESIZE && win->is_resizable) {
            gui_resize_buffer(win, (int)(ev.x / g_ui_scale), (int)(ev.y / g_ui_scale));
        }
        else if (ev.type == GUI_EV_MOUSE_SCROLL) {
            win->scroll_z = ev.keycode;
        }
    }
}

void gui_render(gui_window_t *win) {
    if (!win->shared_mem) return;
    
    int real_w = (int)(win->w * g_ui_scale);
    int real_h = (int)(win->h * g_ui_scale);
    
    memcpy(win->pixels, win->backbuffer, real_w * real_h * 4);
    
    win->shared_mem->is_dirty = 1; 
}

static inline void gui_put_pixel_phys(gui_window_t* win, int px, int py, uint32_t color) {
    int real_w = (int)(win->w * g_ui_scale);
    int real_h = (int)(win->h * g_ui_scale);
    if (px >= 0 && px < real_w && py >= 0 && py < real_h)
        win->backbuffer[py * real_w + px] = color;
}

void gui_put_pixel(gui_window_t* win, int x, int y, uint32_t color) {
    int px_start = (int)(x * g_ui_scale);
    int py_start = (int)(y * g_ui_scale);
    
    if (g_ui_scale <= 1.0f) {
        gui_put_pixel_phys(win, px_start, py_start, color);
    } else {
        int px_end = (int)((x + 1) * g_ui_scale);
        int py_end = (int)((y + 1) * g_ui_scale);
        for (int py = py_start; py < py_end; py++) {
            for (int px = px_start; px < px_end; px++) {
                gui_put_pixel_phys(win, px, py, color);
            }
        }
    }
}

void gui_draw_rect(gui_window_t *win, int x, int y, int w, int h, uint32_t color) {
    int px_start = (int)(x * g_ui_scale);
    int py_start = (int)(y * g_ui_scale);
    int px_w = (int)(w * g_ui_scale);
    int px_h = (int)(h * g_ui_scale);
    
    if (w > 0 && px_w == 0) px_w = 1;
    if (h > 0 && px_h == 0) px_h = 1;

    int real_w = (int)(win->w * g_ui_scale);
    int real_h = (int)(win->h * g_ui_scale);

    if (px_start < 0) { px_w += px_start; px_start = 0; }
    if (py_start < 0) { px_h += py_start; py_start = 0; }
    if (px_start + px_w > real_w) px_w = real_w - px_start;
    if (py_start + px_h > real_h) px_h = real_h - py_start;

    if (px_w <= 0 || px_h <= 0) return;

    for (int py = 0; py < px_h; py++) {
        uint32_t *row_ptr = &win->backbuffer[(py_start + py) * real_w + px_start];
        memset32(row_ptr, color, px_w); 
    }
    
    if (win->shared_mem) win->shared_mem->is_dirty = 1; 
}

void gui_draw_circle_filled(gui_window_t* win, int cx, int cy, int r, uint32_t color) {
    int px_cx = (int)(cx * g_ui_scale);
    int py_cy = (int)(cy * g_ui_scale);
    int px_r = (int)(r * g_ui_scale);
    if (r > 0 && px_r == 0) px_r = 1;

    for (int y = -px_r; y <= px_r; y++)
        for (int x = -px_r; x <= px_r; x++)
            if (x*x + y*y <= px_r*px_r)
                gui_put_pixel_phys(win, px_cx + x, py_cy + y, color);
}

void gui_draw_rounded_rect(gui_window_t* win, int x, int y, int w, int h, int r, uint32_t color) {
    if (r > w/2) r = w/2;
    if (r > h/2) r = h/2;

    gui_draw_rect(win, x, y + r, w, h - 2 * r, color);

    int cx = r, cy = 0;
    int err = 1 - r;

    while (cx >= cy) {
        gui_draw_rect(win, x + r - cx, y + r - cy - 1, w - 2 * r + cx * 2, 1, color);
        gui_draw_rect(win, x + r - cy, y + r - cx - 1, w - 2 * r + cy * 2, 1, color);
        gui_draw_rect(win, x + r - cx, y + h - r + cy, w - 2 * r + cx * 2, 1, color);
        gui_draw_rect(win, x + r - cy, y + h - r + cx, w - 2 * r + cy * 2, 1, color);

        cy++;
        if (err < 0) {
            err += 2 * cy + 1;
        } else {
            cx--;
            err += 2 * (cy - cx) + 1;
        }
    }
}

int gui_button(gui_window_t* win, int x, int y, int w, int h, const char* text, int is_primary) {
    int hovered = (win->mx >= x && win->mx <= x + w && win->my >= y && win->my <= y + h);
    int pressed = hovered && (win->mbtn & 1);
    
    uint32_t bg = is_primary ? (pressed ? 0x000062C9 : 0x00007AFF) : (pressed ? 0x00E0E0E0 : 0x00FFFFFF);
    uint32_t border = is_primary ? 0x00005BB5 : 0x00C4C4C4;
    uint32_t fg = is_primary ? 0x00FFFFFF : 0x00000000;

    gui_draw_rounded_rect(win, x, y, w, h, 4, border);
    gui_draw_rounded_rect(win, x + 1, y + 1, w - 2, h - 2, 3, bg);
    
    int text_w = gui_get_string_width(text, strlen(text));
    gui_draw_string(win, x + (w - text_w) / 2, y + (h - gui_get_font_height()) / 2, text, fg);
    
    return (hovered && win->clicked);
}

int gui_slider(gui_window_t* win, int x, int y, int w, int* percent) {
    int changed = 0;
    if ((win->mbtn & 1) && win->mx >= x - 20 && win->mx <= x + w + 20 && win->my >= y - 20 && win->my <= y + 20) {
        *percent = ((win->mx - x) * 100) / w;
        if (*percent < 0) *percent = 0;
        if (*percent > 100) *percent = 100;
        changed = 1;
    }

    int track_h = 4;
    gui_draw_rounded_rect(win, x, y - track_h/2, w, track_h, 2, 0x00D1D1D6);

    int fill_w = (w * (*percent)) / 100;
    if (fill_w > 4) {
        gui_draw_rounded_rect(win, x, y - track_h/2, fill_w, track_h, 2, 0x00007AFF);
    }
    gui_draw_circle_filled(win, x + fill_w, y, 8, 0x00CCCCCC);
    gui_draw_circle_filled(win, x + fill_w, y, 7, 0x00FFFFFF);

    return changed;
}

int gui_checkbox(gui_window_t* win, int x, int y, int* is_checked, const char* label) {
    int hovered = (win->mx >= x && win->mx <= x + 100 && win->my >= y && win->my <= y + 14);
    if (hovered && win->clicked) *is_checked = !(*is_checked);

    gui_draw_rounded_rect(win, x, y, 14, 14, 2, 0x00C4C4C4);
    if (*is_checked) {
        gui_draw_rounded_rect(win, x + 1, y + 1, 12, 12, 1, 0x00007AFF);
        gui_draw_rect(win, x + 4, y + 7, 2, 2, 0xFFFFFF);
        gui_draw_rect(win, x + 5, y + 8, 2, 2, 0xFFFFFF);
        gui_draw_rect(win, x + 6, y + 7, 2, 2, 0xFFFFFF);
        gui_draw_rect(win, x + 7, y + 6, 2, 2, 0xFFFFFF);
        gui_draw_rect(win, x + 8, y + 5, 2, 2, 0xFFFFFF);
        gui_draw_rect(win, x + 9, y + 4, 2, 2, 0xFFFFFF);
    } else {
        gui_draw_rounded_rect(win, x + 1, y + 1, 12, 12, 1, 0x00FFFFFF);
    }
    
    gui_draw_string(win, x + 20, y, label, 0x00000000);
    return (hovered && win->clicked);
}

int gui_textfield(gui_window_t* win, int x, int y, int w, int h, char* text_buffer, int max_len, int* is_focused) {
    int hovered = (win->mx >= x && win->mx <= x + w && win->my >= y && win->my <= y + h);
    if (win->clicked) {
        *is_focused = hovered;
        if (hovered) {
            int rel_mx = win->mx - (x + 6);
            int current_w = 0;
            int clicked_pos = 0;
            int len = strlen(text_buffer);
            for (int i = 0; i < len; i++) {
                int char_w = font_get_width(text_buffer[i]);
                if (rel_mx < current_w + (char_w/2)) break;
                current_w += char_w;
                clicked_pos++;
            }
            win->cursor_pos = clicked_pos;
        }
    }

    if (*is_focused) {
        int len = strlen(text_buffer);
        if (win->cursor_pos < 0) win->cursor_pos = 0;
        if (win->cursor_pos > len) win->cursor_pos = len;

        if (win->key_code == KEY_LEFT && win->cursor_pos > 0) {
            win->cursor_pos--;
            win->key_code = 0;
        } else if (win->key_code == KEY_RIGHT && win->cursor_pos < len) {
            win->cursor_pos++;
            win->key_code = 0;
        } else if (win->char_input == '\b' && win->cursor_pos > 0) {
            memmove(&text_buffer[win->cursor_pos - 1], &text_buffer[win->cursor_pos], len - win->cursor_pos + 1);
            win->cursor_pos--;
            win->char_input = 0;
        } else if (win->char_input >= 32 && win->char_input <= 126 && len < max_len - 1) {
            memmove(&text_buffer[win->cursor_pos + 1], &text_buffer[win->cursor_pos], len - win->cursor_pos + 1);
            text_buffer[win->cursor_pos] = win->char_input;
            win->cursor_pos++;
            win->char_input = 0;
        }
    }

    gui_draw_rounded_rect(win, x, y, w, h, 3, *is_focused ? 0x00007AFF : 0x00C4C4C4);
    gui_draw_rounded_rect(win, x + 1, y + 1, w - 2, h - 2, 2, 0x00FFFFFF);
    
    gui_draw_string(win, x + 6, y + (h - gui_get_font_height()) / 2, text_buffer, 0x00333333);
    
    if (*is_focused && (get_ticks() / 500) % 2 == 0) {
        int cursor_pixel_x = gui_get_string_width(text_buffer, win->cursor_pos);
        gui_draw_rect(win, x + 6 + cursor_pixel_x, y + 5, 2, h - 10, 0x00007AFF);
    }
    return *is_focused;
}

int gui_textfield_dark(gui_window_t* win, int x, int y, int w, int h, char* text_buffer, int max_len, int* is_focused) {
    int hovered = (win->mx >= x && win->mx <= x + w && win->my >= y && win->my <= y + h);
    if (win->clicked) {
        *is_focused = hovered;
        if (hovered) {
            int rel_mx = win->mx - (x + 6);
            int current_w = 0;
            int clicked_pos = 0;
            int len = strlen(text_buffer);
            for (int i = 0; i < len; i++) {
                int char_w = font_get_width(text_buffer[i]);
                if (rel_mx < current_w + (char_w/2)) break;
                current_w += char_w;
                clicked_pos++;
            }
            win->cursor_pos = clicked_pos;
        }
    }

    if (*is_focused) {
        int len = strlen(text_buffer);
        if (win->cursor_pos < 0) win->cursor_pos = 0;
        if (win->cursor_pos > len) win->cursor_pos = len;

        if (win->key_code == KEY_LEFT && win->cursor_pos > 0) {
            win->cursor_pos--;
            win->key_code = 0;
        } else if (win->key_code == KEY_RIGHT && win->cursor_pos < len) {
            win->cursor_pos++;
            win->key_code = 0;
        } else if (win->char_input == '\b' && win->cursor_pos > 0) {
            memmove(&text_buffer[win->cursor_pos - 1], &text_buffer[win->cursor_pos], len - win->cursor_pos + 1);
            win->cursor_pos--;
            win->char_input = 0;
        } else if (win->char_input >= 32 && win->char_input <= 126 && len < max_len - 1) {
            memmove(&text_buffer[win->cursor_pos + 1], &text_buffer[win->cursor_pos], len - win->cursor_pos + 1);
            text_buffer[win->cursor_pos] = win->char_input;
            win->cursor_pos++;
            win->char_input = 0;
        }
    }

    gui_draw_rounded_rect(win, x, y, w, h, 4, *is_focused ? 0x000A84FF : 0x00444444);
    gui_draw_rounded_rect(win, x + 1, y + 1, w - 2, h - 2, 3, 0x00111111);
    
    gui_draw_string(win, x + 8, y + (h - gui_get_font_height()) / 2, text_buffer, 0x00FFFFFF);
    
    if (*is_focused && (get_ticks() / 500) % 2 == 0) {
        int cursor_pixel_x = gui_get_string_width(text_buffer, win->cursor_pos);
        gui_draw_rect(win, x + 6 + cursor_pixel_x, y + 5, 2, h - 10, 0x00007AFF);
    }
    return *is_focused;
}

void gui_destroy_window(gui_window_t* win) {
    if (shm_map(WM_SHM_KEY) != NULL) {
        int tail = wm_queue->tail;
        wm_queue->commands[tail].type = WM_CMD_DESTROY_WINDOW;
        wm_queue->commands[tail].shm_key = win->shm_key;
        wm_queue->tail = (tail + 1) % 32;
    }
    shm_get(win->shm_key, 0);
    if (win->backbuffer) free(win->backbuffer);
    free(win);
}

void gui_set_resizable(gui_window_t* win, int resizable) {
    win->is_resizable = resizable;
    int tail = wm_queue->tail;
    wm_queue->commands[tail].type = WM_CMD_SET_RESIZABLE;
    wm_queue->commands[tail].shm_key = win->shm_key;
    wm_queue->commands[tail].data = resizable;
    wm_queue->tail = (tail + 1) % 32;
}

void gui_resize_buffer(gui_window_t* win, int new_w, int new_h) {
    int real_w = (int)(new_w * g_ui_scale);
    int real_h = (int)(new_h * g_ui_scale);
    
    shm_key_counter++;
    int new_key = get_ticks() + 20000 + (shm_key_counter * 100);
    
    shm_get(new_key, sizeof(Client_SHM) + (real_w * real_h * 4));
    Client_SHM* new_shm = (Client_SHM*)shm_map(new_key);
    if (!new_shm) {
        shm_get(new_key, 0);
        return;
    }
    
    uint32_t* new_bb = malloc(real_w * real_h * 4);
    if (!new_bb) {
        shm_get(new_key, 0);
        return;
    }
    
    new_shm->event_head = 0;
    new_shm->event_tail = 0;
    memset(new_bb, 0, real_w * real_h * 4);
    
    int tail = wm_queue->tail;
    wm_queue->commands[tail].type = WM_CMD_RESIZE_WINDOW;
    wm_queue->commands[tail].shm_key = win->shm_key;
    wm_queue->commands[tail].w = real_w;
    wm_queue->commands[tail].h = real_h;
    wm_queue->commands[tail].data = new_key;
    wm_queue->tail = (tail + 1) % 32;
    
    if (win->backbuffer) free(win->backbuffer);
    win->shm_key = new_key;
    win->shared_mem = new_shm;
    win->pixels = new_shm->pixels;
    win->backbuffer = new_bb;
    win->w = new_w;
    win->h = new_h;
}
