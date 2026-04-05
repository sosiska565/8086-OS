#ifndef GFX_CONSOLE_H
#define GFX_CONSOLE_H

#include <stdint.h>

void init_gfx_console(void);
void gfx_scroll(void);
void gfx_putc(unsigned int c);
void gfx_print(char *str);
void gfx_set_color(uint32_t fg);
void gfx_set_cursor(int x, int y);
void gfx_get_cursor(int *x, int *y);
void gfx_set_bg_color(uint32_t bg);

#endif