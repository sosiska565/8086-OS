#include "oslib.h"

void print_char(char c){
    __asm__ volatile(
        "int $0x80"
        : 
        : "a"(0), "b"(c) 
    );
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