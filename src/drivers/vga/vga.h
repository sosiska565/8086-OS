#ifndef VGA_H
#define VGA_H

#include<stdint.h>

void clear_screen(void);
void print(const char* str);
void printnumber(int num);
void printhex(unsigned int num);

#endif