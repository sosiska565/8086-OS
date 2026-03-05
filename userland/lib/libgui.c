#include "libgui.h"

void gui_draw_rect(Window *win, int x, int y, int w, int h, uint32_t color) {
    Rect r;
    r.x = x;
    r.y = y;
    r.width = w;
    r.height = h;
    r.color = color;
    draw_rect_filled(&r); 
}

void gui_label(Window *win, int x, int y, char *text, uint32_t color) {
    text_struct ts;
    ts.x = x;
    ts.y = y;
    ts.str = text;
    ts.color = color;
    print_window(win, &ts); 
}

void gui_progress_bar(Window *win, int x, int y, int w, int h, int percent, uint32_t fg_color, uint32_t bg_color) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    
    gui_draw_rect(win, x, y, w, h, bg_color);

    int fill_w = (w * percent) / 100;
    if (fill_w > 0) {
        gui_draw_rect(win, x, y, fill_w, h, fg_color);
    }
}

void gui_panel(Window *win, int x, int y, int w, int h, uint32_t bg_color, uint32_t border_color) {    
    gui_draw_rect(win, x, y, w, 1, border_color); 
    gui_draw_rect(win, x, y + h - 1, w, 1, border_color); 
    gui_draw_rect(win, x, y, 1, h, border_color); 
    gui_draw_rect(win, x + w - 1, y, 1, h, border_color);    
    gui_draw_rect(win, x + 1, y + 1, w - 2, h - 2, bg_color);
}