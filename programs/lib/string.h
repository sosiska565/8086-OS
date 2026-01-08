#ifndef STRING_H
#define STRING_H

#include <stdint.h>

void print_header(int header_bg_color, int header_text_color, char *title);
void print_footer(int footer_bg_color, int footer_text_color, char *text);
void strcpy(char *dst, char *src);
void itoa(int n, char* buffer, int base);

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