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

void cls(){
    __asm__ volatile(
        "int $0x80"
        :
        : "a"(6)
    );
}