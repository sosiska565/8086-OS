#ifndef LIB_H
#define LIB_H

void print_char(char c);
void print(char *str);
void exit(void);
char getc(void);
void gets(char *buffer, int max_len);
void *malloc(int size);
void free(void *ptr);
void cls();

#endif