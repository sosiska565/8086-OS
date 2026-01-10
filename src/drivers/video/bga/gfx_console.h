#ifndef GFX_CONSOLE_H
#define GFX_CONSOLE_H

void init_gfx_console(void);
void gfx_scroll(void);
void gfx_putc(char c);
void gfx_print(char *str);
void gfx_set_color(uint32_t fg);

#endif