#include "oslib.h"

void print_char(char c){
    __asm__ volatile(
        "int $0x80"
        : 
        : "a"(0), "b"(c) 
    );
}

void print_char_colored(char c, int color){
    __asm__ volatile(
        "int $0x80"
        :
        : "a"(7), "b"(c), "c"(color)
    );
}

void print_colored(char *str, int color){
    for(int i = 0; str[i] != '\0'; i++) print_char_colored(str[i], color);
}

void print(char *str){
    for(int i = 0; str[i] != '\0'; i++){
        print_char(str[i]);
    }
}

void exit(void){
    __asm__ volatile(
        "int $0x80"
        : 
        : "a"(1)
    );
}

char getc(void) {
    char c;
    __asm__ volatile(
        "int $0x80"
        : "=a"(c)
        : "a"(2)
    );
    return c;
}

void gets(char *buffer, int max_len){
    __asm__ volatile(
        "int $0x80"
        :
        : "a"(3), "b"(buffer), "c"(max_len)
    );
}

void *malloc(int size){
    void *ptr;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ptr)
        : "a"(4), "b"(size)
    );
    return ptr;
}

void free(void *ptr){
    __asm__ volatile(
        "int $0x80"
        :
        : "a"(5), "b"(ptr)
    );
}

void cls(void){
    __asm__ volatile(
        "int $0x80"
        :
        : "a"(6)
    );
}

unsigned long random(void){
    unsigned long rnd;
    __asm__ volatile(
        "int $0x80"
        : "=a"(rnd)
        : "a"(8)
    );
    return rnd;
}

void print_number(int number){
    __asm__ volatile(
        "int $0x80"
        :
        : "a"(9), "b"(number)
    );
}

unsigned long randmm(unsigned long min, unsigned long max){
    return min + random() % (max - min + 1);
}

void draw_simple_box(char *lines[], char *title, uint8_t centered){
    __asm__ volatile(
        "int $0x80"
        :
        : "a"(10), "b"(lines), "c"(title), "d"(centered)
    );
}

void set_cursor_position(unsigned int x, unsigned int y){
    __asm__ volatile(
        "int $0x80"
        :
        : "a"(11), "b"(x), "c"(y)
    );
}