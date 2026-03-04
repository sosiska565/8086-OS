#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdint.h>

extern int $;
extern unsigned short isReadMode;
extern char* path;

extern uint32_t taskbar_color;
extern uint32_t window_border_color;
extern uint32_t window_active_border_color;


extern uint8_t key_kill;
extern uint8_t key_focus;
extern uint8_t key_console;
extern uint8_t key_layout;
extern uint8_t key_fullscreen;
extern uint8_t key_ws_left;
extern uint8_t key_ws_right;
extern uint8_t key_resize_left;
extern uint8_t key_resize_right;
extern uint8_t key_resize_up;
extern uint8_t key_resize_down;

extern int wm_gaps;
extern int max_grid_cols;
extern int current_workspace;

#endif