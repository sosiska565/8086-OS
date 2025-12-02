#ifndef VGA_H
#define VGA_H

#include<stdint.h>

void clear_screen(void);
void print(const char* str);
void printnumber(int num);
void printhex(unsigned int num);
void print_char(char c);
int strcmp(char *c1, char *c2);

#endif