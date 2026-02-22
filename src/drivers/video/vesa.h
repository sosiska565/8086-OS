#ifndef VESA_H
#define VESA_H

#include <stdint.h>

extern int screen_pitch;
extern uint32_t *video_memory;
extern uint32_t *back_buffer;

void init_vesa();
void put_pixel(int x, int y, uint32_t color, int tar);
void clear_screen_vesa(uint32_t color);
void vesa_draw_char(int x, int y, unsigned int c, uint32_t color, uint32_t bgcolor);
int get_screen_width(void);
int get_screen_height(void);
void vesa_render_buffer();
void vesa_render_rect(int x, int y, int w, int h);

#endif