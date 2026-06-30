#ifndef LIBGUI_H
#define LIBGUI_H

#include <oslib.h>
#include "gui_protocol.h"

typedef struct {
    int shm_key;
    int w, h;
    Client_SHM *shared_mem;
    uint32_t *pixels;       
    uint32_t *backbuffer;   

    int mx, my;             
    int mbtn;               
    int clicked;            
    int closed;             
    char char_input;        
    int key_code; 
    int is_resizable; 
    int scroll_z; 
    int cursor_pos; 
} gui_window_t;


gui_window_t* gui_create_window(const char* title, int w, int h);
int gui_poll_event(gui_window_t *win, GUI_Event *ev); 
void gui_update(gui_window_t *win); 
void gui_render(gui_window_t *win);
void gui_destroy_window(gui_window_t *win);

void gui_put_pixel(gui_window_t *win, int x, int y, uint32_t color);

void gui_draw_rect(gui_window_t *win, int x, int y, int w, int h, uint32_t color);
void gui_draw_char(gui_window_t *win, int x, int y, char c, uint32_t fg);
void gui_draw_string(gui_window_t *win, int x, int y, const char *str, uint32_t fg);
void gui_draw_circle_filled(gui_window_t *win, int cx, int cy, int r, uint32_t color);
void gui_draw_rounded_rect(gui_window_t *win, int x, int y, int w, int h, int r, uint32_t color);



int gui_button(gui_window_t *win, int x, int y, int w, int h, const char *text, int is_primary);
int gui_slider(gui_window_t *win, int x, int y, int w, int *percent);
int gui_checkbox(gui_window_t *win, int x, int y, int *is_checked, const char *label);
int gui_textfield(gui_window_t *win, int x, int y, int w, int h, char *text_buffer, int max_len, int *is_focused);

void gui_set_resizable(gui_window_t *win, int resizable);
void gui_resize_buffer(gui_window_t *win, int new_w, int new_h);

gui_window_t* gui_create_frameless(int x, int y, int w, int h);

int gui_textfield_dark(gui_window_t *win, int x, int y, int w, int h, char *text_buffer, int max_len, int *is_focused);

#endif