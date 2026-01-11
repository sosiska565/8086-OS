#include <oslib.h>
#include <stdarg.h>

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

void set_cursor_position(unsigned int x, unsigned int y){
    __asm__ volatile(
        "int $0x80"
        :
        : "a"(11), "b"(x), "c"(y)
    );
}

void set_background_color(vga_color_t color){
    __asm__ volatile(
        "int $0x80"
        :
        : "a"(12), "b"(color)
    );
}

void set_text_color(vga_color_t color){
    __asm__ volatile(
        "int $0x80"
        :
        : "a"(13), "b"(color)
    );
}

void get_cursor_xy(unsigned int *x, unsigned int *y){
    __asm__ volatile(
        "int $0x80"
        :
        : "a"(14), "b"(x), "c"(y)
    );
}

vga_color_t get_current_color(void){
    vga_color_t color;
    __asm__ volatile(
        "int $0x80"
        : "=a"(color)
        : "a"(15)
    );
    return color;
}

void set_current_color(vga_color_t color){
    __asm__ volatile(
        "int $0x80"
        :
        : "a"(16), "b"(color)
    );
}

int read_file(char *file_name, uint8_t *file_buffer){
    int sizefile;
    __asm__ volatile(
        "int $0x80"
        : "=a"(sizefile)
        : "a"(10), "b"(file_name), "c"(file_buffer)
    );
    return sizefile;
}

void printhex(unsigned int num){
    __asm__ volatile(
        "int $0x80"
        :
        : "a"(17), "b"(num)
    );
}

int get_file_size(char *file_name){
    int size;
    __asm__ volatile(
        "int $0x80"
        : "=a"(size)
        : "a"(18), "b"(file_name)
    );
    return size;
}

uint8_t get_scanecode(void){
    int scanecode;
    __asm__ volatile(
        "int $0x80"
        : "=a"(scanecode)
        : "a"(19)
    );
    return scanecode;
}

int write_file(char *filename, uint8_t *buffer, uint32_t size){
    int error;
    __asm__ volatile(
        "int $0x80"
        : "=a"(error)
        : "a"(20), "b"(filename), "c"(buffer), "d"(size)
    );
    return error;
}

char scancode_to_ascii(uint8_t scancode){
    char c;
    __asm__ volatile(
        "int $0x80"
        : "=a"(c)
        : "a"(21), "b"(scancode)
    );
    return c;
}

void my_memcpy(void* dest, void* src, int size) {
    uint8_t* d = (uint8_t*)dest;
    uint8_t* s = (uint8_t*)src;
    for(int i = 0; i < size; i++) {
        d[i] = s[i];
    }
}

int getScreenWidth(void){
    int width;
    __asm__ volatile(
        "int $0x80"
        : "=a"(width)
        : "a"(22)
    );
    return width;
}

int getScreenHeight(void){
    int height;
    __asm__ volatile(
        "int $0x80"
        : "=a"(height)
        : "a"(23)
    );
    return height;
}

void printf(const char* format, ...){
    va_list args;
    va_start(args, format);
    __asm__ volatile(
        "int $0x80"
        :
        : "a"(24), "b"(format), "c"(args)
    );

    va_end(args);
}