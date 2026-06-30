#include <oslib.h>
#include "font.h"
#include "gui_protocol.h"
#include <signal.h>

#define COLOR_BG           0x001F2B5F
#define COLOR_TITLE_ACTIVE 0x00323233 
#define COLOR_TITLE        0x00252526 
#define COLOR_BORDER       0x00454545 
#define TITLE_BAR_H        26         


#define MOD_SHIFT 1
#define MOD_CTRL  2
#define MOD_ALT   4
#define MOD_WIN   8


#define TERMINAL "path/gterm.elf"

typedef struct {
    int shm_key;
    int x, y, w, h;
    int buf_w, buf_h; 
    int is_resizable; 
    int is_frameless; 
    char title[32];
    Client_SHM *shared_mem;
} Window;

Window windows[16]; 
int num_windows = 0;
int sw, sh, sbpp; 
uint32_t *wm_backbuffer; 
WM_Queue *wm_queue;

uint32_t *scaled_wallpaper = NULL;

int drag_win_index = -1; int drag_offset_x = 0; int drag_offset_y = 0;
int resize_win_index = -1;

#define RM_NONE 0
#define RM_N 1
#define RM_S 2
#define RM_E 3
#define RM_W 4
#define RM_NW 5
#define RM_NE 6
#define RM_SW 7
#define RM_SE 8

int resize_mode = RM_NONE;
int orig_x, orig_y, orig_w, orig_h, start_mx, start_my;
int current_cursor_type = RM_NONE;

void wm_put_pixel(int x, int y, uint32_t color) { if (x >= 0 && x < sw && y >= 0 && y < sh) wm_backbuffer[y * sw + x] = color; }
void wm_draw_rect(int x, int y, int w, int h, uint32_t color) { for (int iy = 0; iy < h; iy++) for (int ix = 0; ix < w; ix++) wm_put_pixel(x + ix, y + iy, color); }
void wm_draw_char(int x, int y, char c, uint32_t fg) { if ((unsigned char)c > 127) c = '?'; char *glyph = font8x8_basic[(int)c]; for (int ry = 0; ry < 8; ry++) for (int rx = 0; rx < 8; rx++) if ((glyph[ry] >> rx) & 1) wm_put_pixel(x + rx, y + ry, fg); }
void wm_draw_circle(int cx, int cy, int r, uint32_t color) { for (int y = -r; y <= r; y++) for (int x = -r; x <= r; x++) if (x*x + y*y <= r*r) wm_put_pixel(cx + x, cy + y, color); }

void wm_draw_string(int x, int y, const char *str, uint32_t fg) { 
    int cx = x; 
    while (*str) { 
        wm_draw_char(cx, y, *str, fg); 
        cx += font_get_width(*str); 
        str++;
    } 
}

void load_wallpaper(const char *path) {
    int sz = get_file_size(path);
    if (sz <= 54) return;

    uint8_t *file_buf = malloc(sz);
    if (!file_buf) return;
    read_file(path, file_buf);

    if (file_buf[0] != 'B' || file_buf[1] != 'M') { free(file_buf); return; }

    uint32_t data_offset = *(uint32_t*)(&file_buf[10]);
    int width = *(int32_t*)(&file_buf[18]);
    int height = *(int32_t*)(&file_buf[22]);
    uint16_t bpp = *(uint16_t*)(&file_buf[28]);

    int top_down = 0;
    if (height < 0) { height = -height; top_down = 1; }

    if (bpp != 24 && bpp != 32) { free(file_buf); return; }

    uint32_t *temp_buf = malloc(width * height * 4);
    if (!temp_buf) { free(file_buf); return; }

    int row_bytes = ((width * bpp + 31) / 32) * 4;

    for (int y = 0; y < height; y++) {
        int src_y = top_down ? y : (height - 1 - y); 
        uint8_t *row_ptr = file_buf + data_offset + (src_y * row_bytes);
        
        for (int x = 0; x < width; x++) {
            uint8_t b = row_ptr[x * (bpp / 8) + 0];
            uint8_t g = row_ptr[x * (bpp / 8) + 1];
            uint8_t r = row_ptr[x * (bpp / 8) + 2];
            temp_buf[y * width + x] = (r << 16) | (g << 8) | b;
        }
    }
    free(file_buf);

    if (scaled_wallpaper) free(scaled_wallpaper);
    scaled_wallpaper = malloc(sw * sh * 4);
    if (!scaled_wallpaper) { free(temp_buf); return; }

    for (int y = 0; y < sh; y++) {
        int src_y = (y * height) / sh;
        uint32_t *src_row = &temp_buf[src_y * width];
        uint32_t *dst_row = &scaled_wallpaper[y * sw];
        for (int x = 0; x < sw; x++) {
            int src_x = (x * width) / sw;
            dst_row[x] = src_row[src_x];
        }
    }
    free(temp_buf);
}

void send_event_to_client(Client_SHM *client, int type, int x, int y, int data) {
    if(!client) return;
    int tail = client->event_tail;
    client->events[tail].type = type; client->events[tail].x = x; client->events[tail].y = y;
    if (type == GUI_EV_KEY_PRESS) client->events[tail].keycode = data; else client->events[tail].button = data;
    client->event_tail = (tail + 1) % 32;
}

void process_wm_queue() {
    while (wm_queue->head != wm_queue->tail) {
        WM_Command cmd = wm_queue->commands[wm_queue->head];
        wm_queue->head = (wm_queue->head + 1) % 32;

        if (cmd.type == WM_CMD_CREATE_WINDOW && num_windows < 16) {
            int idx = num_windows++;
            windows[idx].shm_key = cmd.shm_key; windows[idx].w = cmd.w; windows[idx].h = cmd.h;
            windows[idx].buf_w = cmd.w; windows[idx].buf_h = cmd.h; windows[idx].is_resizable = 0;
            windows[idx].is_frameless = 0; 
            windows[idx].x = (sw / 2 - cmd.w / 2) + (idx * 20); windows[idx].y = (sh / 2 - cmd.h / 2) + (idx * 20);
            strcpy(windows[idx].title, cmd.title);
            windows[idx].shared_mem = (Client_SHM*)shm_map(cmd.shm_key);
        }
        else if (cmd.type == WM_CMD_SET_RESIZABLE) {
            for (int i = 0; i < num_windows; i++) if (windows[i].shm_key == cmd.shm_key) { windows[i].is_resizable = cmd.data; break; }
        }
        else if (cmd.type == WM_CMD_RESIZE_WINDOW) {
            for (int i=0; i<num_windows; i++) {
                if (windows[i].shm_key == cmd.shm_key) {
                    int old_key = windows[i].shm_key;
                    windows[i].shm_key = cmd.data; 
                    
                    
                    windows[i].w = cmd.w;       
                    windows[i].h = cmd.h;       
                    windows[i].buf_w = cmd.w; 
                    windows[i].buf_h = cmd.h;
                    
                    windows[i].shared_mem = (Client_SHM*)shm_map(cmd.data);
                    
                    
                    shm_get(old_key, 0); 
                    break;
                }
            }
        }
        else if (cmd.type == WM_CMD_DESTROY_WINDOW) {
            for (int i = 0; i < num_windows; i++) {
                if (windows[i].shm_key == cmd.shm_key) {
                    for (int j = i; j < num_windows - 1; j++) windows[j] = windows[j + 1];
                    num_windows--; break;
                }
            }
        }
        else if (cmd.type == WM_CMD_CREATE_FRAMELESS && num_windows < 16) {
            int idx = num_windows++;
            windows[idx].shm_key = cmd.shm_key; 
            windows[idx].w = cmd.w; windows[idx].h = cmd.h;
            windows[idx].buf_w = cmd.w; windows[idx].buf_h = cmd.h;
            windows[idx].x = cmd.x; windows[idx].y = cmd.y; 
            windows[idx].is_resizable = 0;
            windows[idx].is_frameless = 1; 
            windows[idx].shared_mem = (Client_SHM*)shm_map(cmd.shm_key);
        }
        else if (cmd.type == WM_CMD_SET_WALLPAPER) {
            load_wallpaper(cmd.title);
        }
    }
}


void compose_screen(int mx, int my) {
    
    if (scaled_wallpaper) {
        memcpy(wm_backbuffer, scaled_wallpaper, sw * sh * 4);
    } else {
        
        for(int i = 0; i < sw * sh; i++) wm_backbuffer[i] = COLOR_BG;
    }

    
    for (int i = 0; i < num_windows; i++) {
        Window *win = &windows[i];
        if (!win->shared_mem) continue; 
        uint32_t *client_pixels = win->shared_mem->pixels;
        if (!client_pixels) continue; 

        if (!win->is_frameless) {
            int is_active = (i == num_windows - 1);
            uint32_t t_color = is_active ? COLOR_TITLE_ACTIVE : COLOR_TITLE;

            
            int title_start_y = win->y - TITLE_BAR_H;
            if (title_start_y < 0) title_start_y = 0;
            for (int r = title_start_y; r < win->y; r++) {
                if (r >= sh) break;
                int draw_x = (win->x < 0) ? 0 : win->x;
                int draw_w = win->w - ((win->x < 0) ? -win->x : 0);
                if (draw_x + draw_w > sw) draw_w = sw - draw_x;
                if (draw_w > 0) {
                    
                    for(int c = 0; c < draw_w; c++) wm_backbuffer[r * sw + draw_x + c] = t_color;
                }
            }

            
            for (int r = 0; r < win->h; r++) {
                int screen_y = win->y + r;
                if (screen_y < 0) continue;
                if (screen_y >= sh) break;
                
                
                if (r >= win->buf_h) continue; 

                int src_x = 0;
                int dest_x = win->x;
                int copy_w = win->w;

                if (dest_x < 0) { src_x = -dest_x; copy_w += dest_x; dest_x = 0; }
                if (dest_x + copy_w > sw) { copy_w = sw - dest_x; }
                
                
                if (src_x + copy_w > win->buf_w) copy_w = win->buf_w - src_x;

                if (copy_w > 0) {
                    
                    memcpy(&wm_backbuffer[screen_y * sw + dest_x], 
                           &client_pixels[r * win->buf_w + src_x], 
                           copy_w * 4);
                }
            }
            
            
            int btn_y = win->y - TITLE_BAR_H + (TITLE_BAR_H / 2); 
            wm_draw_circle(win->x + 15, btn_y, 5, is_active ? 0x00FF5F56 : 0x004D4D4D);
            wm_draw_circle(win->x + 35, btn_y, 5, is_active ? 0x00FFBD2E : 0x004D4D4D);
            wm_draw_circle(win->x + 55, btn_y, 5, is_active ? 0x0027C93F : 0x004D4D4D);
            
            
            int text_w = 0; for(int k=0; win->title[k]; k++) text_w += font_get_width(win->title[k]);
            int title_x = win->x + (win->w - text_w) / 2;
            if (title_x > win->x + 70) wm_draw_string(title_x, win->y - TITLE_BAR_H + 9, win->title, is_active ? 0x00CCCCCC : 0x00888888);

        } else {
            
            for (int r = 0; r < win->h; r++) {
                int screen_y = win->y + r;
                if (screen_y < 0 || screen_y >= sh) continue;
                if (r >= win->buf_h) continue; 

                for (int c = 0; c < win->w; c++) {
                    int screen_x = win->x + c;
                    if (screen_x < 0 || screen_x >= sw) continue;
                    if (c >= win->buf_w) continue; 

                    uint32_t pixel = client_pixels[r * win->buf_w + c];
                    if (pixel != 0x00FF00FF) wm_backbuffer[screen_y * sw + screen_x] = pixel;
                }
            }
        }
    }

    
    static const uint8_t cur_normal[15][10] = { {2,0,0,0,0,0,0,0,0,0}, {2,2,0,0,0,0,0,0,0,0}, {2,1,2,0,0,0,0,0,0,0}, {2,1,1,2,0,0,0,0,0,0}, {2,1,1,1,2,0,0,0,0,0}, {2,1,1,1,1,2,0,0,0,0}, {2,1,1,1,1,1,2,0,0,0}, {2,1,1,1,1,1,1,2,0,0}, {2,1,1,1,1,1,1,1,2,0}, {2,1,1,1,1,1,2,2,2,2}, {2,1,1,2,1,1,2,0,0,0}, {2,1,2,0,2,1,1,2,0,0}, {2,2,0,0,2,1,1,2,0,0}, {2,0,0,0,0,2,2,0,0,0}, {0,0,0,0,0,0,0,0,0,0} };
    static const uint8_t cur_ns[15][10] =     { {0,0,0,0,2,0,0,0,0,0}, {0,0,0,2,1,2,0,0,0,0}, {0,0,2,1,1,1,2,0,0,0}, {0,2,1,1,1,1,1,2,0,0}, {0,0,0,2,1,2,0,0,0,0}, {0,0,0,2,1,2,0,0,0,0}, {0,0,0,2,1,2,0,0,0,0}, {0,0,0,2,1,2,0,0,0,0}, {0,0,0,2,1,2,0,0,0,0}, {0,0,0,2,1,2,0,0,0,0}, {0,0,0,2,1,2,0,0,0,0}, {0,2,1,1,1,1,1,2,0,0}, {0,0,2,1,1,1,2,0,0,0}, {0,0,0,2,1,2,0,0,0,0}, {0,0,0,0,2,0,0,0,0,0} };
    static const uint8_t cur_ew[15][10] =     { {0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0}, {0,0,0,2,0,0,2,0,0,0}, {0,0,2,1,2,2,1,2,0,0}, {0,2,1,1,1,1,1,1,2,0}, {2,1,1,1,1,1,1,1,1,2}, {0,2,1,1,1,1,1,1,2,0}, {0,0,2,1,2,2,1,2,0,0}, {0,0,0,2,0,0,2,0,0,0}, {0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0} };
    static const uint8_t cur_nwse[15][10] =   { {2,2,2,2,2,0,0,0,0,0}, {2,1,1,1,2,0,0,0,0,0}, {2,1,1,2,0,0,0,0,0,0}, {2,1,2,1,2,0,0,0,0,0}, {2,2,0,2,1,2,0,0,0,0}, {0,0,0,0,2,1,2,0,0,0}, {0,0,0,0,0,2,1,2,0,0}, {0,0,0,0,0,0,2,1,2,0}, {0,0,0,0,0,0,0,2,2,2}, {0,0,0,0,0,0,0,0,2,2}, {0,0,0,0,0,0,0,0,0,2}, {0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0} };
    static const uint8_t cur_nesw[15][10] =   { {0,0,0,0,0,2,2,2,2,2}, {0,0,0,0,0,2,1,1,1,2}, {0,0,0,0,0,0,2,1,1,2}, {0,0,0,0,0,0,2,1,2,2}, {0,0,0,0,0,2,1,2,0,2}, {0,0,0,0,2,1,2,0,0,0}, {0,0,0,2,1,2,0,0,0,0}, {0,0,2,1,2,0,0,0,0,0}, {2,2,2,0,0,0,0,0,0,0}, {2,2,0,0,0,0,0,0,0,0}, {2,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0} };

    const uint8_t (*cbmp)[10] = cur_normal;
    if (current_cursor_type == RM_N || current_cursor_type == RM_S) cbmp = cur_ns;
    else if (current_cursor_type == RM_E || current_cursor_type == RM_W) cbmp = cur_ew;
    else if (current_cursor_type == RM_NW || current_cursor_type == RM_SE) cbmp = cur_nwse;
    else if (current_cursor_type == RM_NE || current_cursor_type == RM_SW) cbmp = cur_nesw;

    for(int cy = 0; cy < 15; cy++) {
        for(int cx = 0; cx < 10; cx++) { 
            if (cbmp[cy][cx] == 1) wm_put_pixel(mx + cx, my + cy, 0x00000000); 
            else if (cbmp[cy][cx] == 2) wm_put_pixel(mx + cx, my + cy, 0x00FFFFFF); 
        }
    }
}

int main(int argc, char** argv) {
    get_screen_info(&sw, &sh, &sbpp);
    wm_backbuffer = malloc(sw * sh * 4);
    font_calc_widths();

    signal(SIGINT, SIG_IGN);

    shm_get(WM_SHM_KEY, sizeof(WM_Queue)); wm_queue = (WM_Queue*)shm_map(WM_SHM_KEY);
    wm_queue->head = 0; wm_queue->tail = 0;
    
    load_wallpaper("/wallpapers/wallpaper.bmp");

    int mx = sw / 2, my = sh / 2, mbtn = 0, mz = 0, prev_mbtn = 0;
    static int prev_mx = -1, prev_my = -1;


    while (1) {
        process_wm_queue();

        for (int i = 0; i < num_windows; i++) {
            if (shm_get(windows[i].shm_key, -1) == 0) {
                for (int j = i; j < num_windows - 1; j++) windows[j] = windows[j + 1];
                num_windows--;
                if (drag_win_index == i) drag_win_index = -1;
                if (resize_win_index == i) resize_win_index = -1;
                i--; 
            }
        }

        get_mouse(&mx, &my, &mbtn, &mz);
        int left_click = mbtn & 1; int right_click = mbtn & 2; 
        int clicked_just_now = (left_click && !prev_mbtn);
        int right_clicked_just_now = (right_click && !(prev_mbtn & 2));
        int released_just_now = (!left_click && (prev_mbtn & 1));

        if (mx != prev_mx || my != prev_my) {
            if (drag_win_index == -1 && resize_win_index == -1 && num_windows > 0) {
                Window *active = &windows[num_windows - 1];
                if (mx >= active->x && mx <= active->x + active->w && my >= active->y && my <= active->y + active->h) 
                    send_event_to_client(active->shared_mem, GUI_EV_MOUSE_MOVE, mx - active->x, my - active->y, mbtn);
            }
            prev_mx = mx; prev_my = my;
        }

        int hm = RM_NONE;
        if (num_windows > 0 && drag_win_index == -1 && resize_win_index == -1) {
            Window *win = &windows[num_windows - 1];
            if (!win->is_frameless && win->is_resizable) {
                int r_l = (mx >= win->x - 2 && mx <= win->x + 8);
                int r_r = (mx >= win->x + win->w - 8 && mx <= win->x + win->w + 2);
                int r_t = (my >= win->y - TITLE_BAR_H - 2 && my <= win->y - TITLE_BAR_H + 8);
                int r_b = (my >= win->y + win->h - 8 && my <= win->y + win->h + 2);
                
                if (r_t && r_l) hm = RM_NW;
                else if (r_t && r_r) hm = RM_NE;
                else if (r_b && r_l) hm = RM_SW;
                else if (r_b && r_r) hm = RM_SE;
                else if (r_t) hm = RM_N;
                else if (r_b) hm = RM_S;
                else if (r_l) hm = RM_W;
                else if (r_r) hm = RM_E;
            }
        }
        current_cursor_type = hm;

        if (clicked_just_now || right_clicked_just_now) {
            int button_id = clicked_just_now ? 1 : 2; 
            for (int i = num_windows - 1; i >= 0; i--) {
                Window *win = &windows[i];
                if (mx >= win->x - 2 && mx <= win->x + win->w + 2 && my >= win->y - TITLE_BAR_H - 2 && my <= win->y + win->h + 2) {
                    
                    if (!win->is_frameless) {
                        if (button_id == 1 && my >= win->y - TITLE_BAR_H && my <= win->y && mx >= win->x + 8 && mx <= win->x + 22) { send_event_to_client(win->shared_mem, GUI_EV_CLOSE, 0, 0, 0); break; }
                        
                        if (button_id == 1 && hm != RM_NONE && i == num_windows - 1) {
                            resize_win_index = i;
                            resize_mode = hm;
                            orig_x = win->x; orig_y = win->y; orig_w = win->w; orig_h = win->h;
                            start_mx = mx; start_my = my;
                            break;
                        }
                        
                        if (button_id == 1 && my >= win->y - TITLE_BAR_H && my <= win->y && hm == RM_NONE) {
                            drag_offset_x = mx - win->x; drag_offset_y = my - win->y;
                            if (i != num_windows - 1) { Window t = windows[i]; for (int j = i; j < num_windows - 1; j++) windows[j] = windows[j + 1]; windows[num_windows - 1] = t; }
                            drag_win_index = num_windows - 1; break;
                        }
                    }
                    
                    if (my >= win->y && my <= win->y + win->h && mx >= win->x && mx <= win->x + win->w) {
                        if (i != num_windows - 1) { Window t = windows[i]; for (int j = i; j < num_windows - 1; j++) windows[j] = windows[j + 1]; windows[num_windows - 1] = t; }
                        send_event_to_client(windows[num_windows - 1].shared_mem, GUI_EV_MOUSE_CLICK, mx - win->x, my - win->y, button_id); break; 
                    }
                }
            }
        }

        if (left_click && resize_win_index != -1) {
            int dx = mx - start_mx;
            int dy = my - start_my;
            int nx = orig_x, ny = orig_y, nw = orig_w, nh = orig_h;

            if (resize_mode == RM_E || resize_mode == RM_NE || resize_mode == RM_SE) nw += dx;
            if (resize_mode == RM_W || resize_mode == RM_NW || resize_mode == RM_SW) { nw -= dx; nx += dx; }
            if (resize_mode == RM_S || resize_mode == RM_SW || resize_mode == RM_SE) nh += dy;
            if (resize_mode == RM_N || resize_mode == RM_NW || resize_mode == RM_NE) { nh -= dy; ny += dy; }

            if (nw < 150) { nx = orig_x + (orig_w - 150); nw = 150; } 
            if (nh < 100) { ny = orig_y + (orig_h - 100); nh = 100; }

            if (resize_mode == RM_W || resize_mode == RM_NW || resize_mode == RM_SW) {
                if (orig_w - dx < 150) nx = orig_x + orig_w - 150; 
            }
            if (resize_mode == RM_N || resize_mode == RM_NW || resize_mode == RM_NE) {
                if (orig_h - dy < 100) ny = orig_y + orig_h - 100; 
            }

            windows[resize_win_index].x = nx;
            windows[resize_win_index].y = ny;
            windows[resize_win_index].w = nw;
            windows[resize_win_index].h = nh;
        } else if (left_click && drag_win_index != -1) {
            windows[drag_win_index].x = mx - drag_offset_x; windows[drag_win_index].y = my - drag_offset_y;
        }

        if (mz != 0 && num_windows > 0 && drag_win_index == -1 && resize_win_index == -1) {
            Window *active = &windows[num_windows - 1];
            if (mx >= active->x && mx <= active->x + active->w && my >= active->y && my <= active->y + active->h) {
                send_event_to_client(active->shared_mem, GUI_EV_MOUSE_SCROLL, 0, 0, mz);
            }
        }

        if (released_just_now) {
            if (resize_win_index != -1) {
                send_event_to_client(windows[resize_win_index].shared_mem, GUI_EV_RESIZE, windows[resize_win_index].w, windows[resize_win_index].h, 0);
            }
            drag_win_index = -1; resize_win_index = -1; resize_mode = RM_NONE; current_cursor_type = RM_NONE;
        }
        prev_mbtn = mbtn;

        unsigned int key;
        while ((key = poll_key()) != 0) {
            uint8_t mods = get_key_modifiers(); 
            char c = (char)(key & 0xFF); 
            
            if ((mods & MOD_ALT) && (mods & MOD_CTRL) && (c == 27 || key == KEY_ESC)) {
                for (int i = 0; i < num_windows; i++) {
                    send_event_to_client(windows[i].shared_mem, GUI_EV_CLOSE, 0, 0, 0);
                }
                uint32_t timeout = get_ticks() + 500;
                while(get_ticks() < timeout) yield();

                shm_get(WM_SHM_KEY, 0);
                if (scaled_wallpaper) free(scaled_wallpaper);
                free(wm_backbuffer);
                clear_screen(); 
                exit(0); 
            }
            int handled_by_wm = 0;
            if ((mods & MOD_ALT) && (c == 't' || c == 'T')) { spawn(TERMINAL, NULL, NULL); handled_by_wm = 1; }
            else if ((mods & MOD_ALT) && (c == 'q' || c == 'Q')) { if (num_windows > 0) send_event_to_client(windows[num_windows - 1].shared_mem, GUI_EV_CLOSE, 0, 0, 0); handled_by_wm = 1; }
            else if ((mods & MOD_ALT) && (c == 'n' || c == 'N')) { if (num_windows > 1) { Window t = windows[0]; for (int j = 0; j < num_windows - 1; j++) windows[j] = windows[j + 1]; windows[num_windows - 1] = t; } handled_by_wm = 1; }
            
            if (!handled_by_wm && num_windows > 0) {
                send_event_to_client(windows[num_windows - 1].shared_mem, GUI_EV_KEY_PRESS, 0, 0, key);
            }
        }

        char current_title[32] = "";
        for (int i = num_windows - 1; i >= 0; i--) {
            if (!windows[i].is_frameless) {
                strcpy(current_title, windows[i].title);
                break;
            }
        }
        strcpy(wm_queue->active_window_title, current_title);

        compose_screen(mx, my); flush_screen(wm_backbuffer); yield();
    }
    return 0;
}