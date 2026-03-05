#ifndef LIBGUI_H
#define LIBGUI_H

#include <stdint.h>
#include "oslib.h"

void gui_draw_rect(Window *win, int x, int y, int w, int h, uint32_t color);
void gui_label(Window *win, int x, int y, char *text, uint32_t color);

void gui_progress_bar(Window *win, int x, int y, int w, int h, int percent, uint32_t fg_color, uint32_t bg_color);
void gui_panel(Window *win, int x, int y, int w, int h, uint32_t bg_color, uint32_t border_color);

#endif