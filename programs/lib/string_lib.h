#ifndef STRINGS_H
#define STRINGS_H

#include <stdint.h>

static const uint32_t vga_to_rgb[] = {
    0x000000,
    0x0000AA,
    0x00AA00,
    0x00AAAA,
    0xAA0000, 
    0xAA00AA,
    0xAA5500,
    0xAAAAAA,
    0x555555,
    0x5555FF,
    0x55FF55,
    0x55FFFF,
    0xFF5555,
    0xFF55FF,
    0xFFFF55,
    0xFFFFFF
};

void print_header(int header_bg_color, int header_text_color, char *title);
void print_footer(int footer_bg_color, int footer_text_color, char *text);
void strcpy(char *dst, char *src);
void itoa(unsigned int n, char* buffer, int base);
char *strcat(char *dest, const char *src);
int strlen(char *str);

//псевдо графика

void draw_text_box_ex(char* lines[], char* title, 
                      uint8_t padding_top, uint8_t padding_bottom,
                      uint8_t padding_left, uint8_t padding_right,
                      uint8_t border_color, uint8_t text_color, uint8_t title_color,
                      uint8_t centered);
void draw_text_box(char* lines[], char* title, uint8_t padding, 
                   uint8_t border_color, uint8_t text_color, uint8_t title_color,
                   uint8_t centered);
void draw_simple_box(char* lines[], char* title, uint8_t centered);

#endif