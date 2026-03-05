#include <oslib.h>
#include <stdarg.h>
#include "string_lib.h"

void print_char(unsigned int c){
    __asm__ volatile("int $0x80" : : "a"(0), "b"(c));
}
void print_char_colored(char c, int color){
    __asm__ volatile("int $0x80" : : "a"(7), "b"(c), "c"(color));
}
void print_colored(char *str, int color){
    for(int i = 0; str[i] != '\0'; i++) print_char_colored(str[i], color);
}
void print(char *str){
    write_file(stdout, (uint8_t*)str, strlen(str));
}
void exit(void){
    __asm__ volatile("int $0x80" : : "a"(1));
}

unsigned int getc(void) {
    uint8_t buf[1];
    read_file(stdin, buf);
    return buf[0];
}

void gets(char *buffer, int max_len){
    int pos = 0;
    while(1) {
        unsigned int c = getc();
        if (c == '\n') {
            buffer[pos] = '\0';
            print("\n");
            return;
        } else if (c == '\b') {
            if (pos > 0) {
                pos--;
                while (pos > 0 && (buffer[pos] & 0xC0) == 0x80) pos--;
                buffer[pos] = '\0';
                print("\b \b");
            }
        } else if (c != 0) {
            char tmp[4];
            int bytes = 0;
            if (c < 0x80) { tmp[0] = c; bytes = 1; }
            else if (c < 0x800) { tmp[0] = 0xC0 | (c >> 6); tmp[1] = 0x80 | (c & 0x3F); bytes = 2; }
            else { tmp[0] = 0xE0 | (c >> 12); tmp[1] = 0x80 | ((c >> 6) & 0x3F); tmp[2] = 0x80 | (c & 0x3F); bytes = 3; }
            
            if (pos + bytes < max_len - 1) {
                for(int i=0; i<bytes; i++) buffer[pos++] = tmp[i];
                print_char(c);
            }
        }
    }
}

void *malloc(int size){
    void *ptr;
    __asm__ volatile("int $0x80" : "=a"(ptr) : "a"(4), "b"(size));
    return ptr;
}
void free(void *ptr){
    __asm__ volatile("int $0x80" : : "a"(5), "b"(ptr));
}
void cls(void){
    __asm__ volatile("int $0x80" : : "a"(6));
}
unsigned long random(void){
    unsigned long rnd;
    __asm__ volatile("int $0x80" : "=a"(rnd) : "a"(8));
    return rnd;
}
void print_number(int number){
    __asm__ volatile("int $0x80" : : "a"(9), "b"(number));
}
unsigned long randmm(unsigned long min, unsigned long max){
    return min + random() % (max - min + 1);
}
void set_cursor_position(unsigned int x, unsigned int y){
    __asm__ volatile("int $0x80" : : "a"(11), "b"(x), "c"(y));
}
void set_background_color(vga_color_t color){
    __asm__ volatile("int $0x80" : : "a"(12), "b"(color));
}
void set_text_color(vga_color_t color){
    __asm__ volatile("int $0x80" : : "a"(13), "b"(color));
}
void get_cursor_xy(unsigned int *x, unsigned int *y){
    __asm__ volatile("int $0x80" : : "a"(14), "b"(x), "c"(y));
}
vga_color_t get_current_color(void){
    vga_color_t color;
    __asm__ volatile("int $0x80" : "=a"(color) : "a"(15));
    return color;
}
void set_current_color(vga_color_t color){
    __asm__ volatile("int $0x80" : : "a"(16), "b"(color));
}
int read_file(char *file_name, uint8_t *file_buffer){
    int sizefile;
    __asm__ volatile("int $0x80" : "=a"(sizefile) : "a"(10), "b"(file_name), "c"(file_buffer));
    return sizefile;
}
void printhex(unsigned int num){
    __asm__ volatile("int $0x80" : : "a"(17), "b"(num));
}
int get_file_size(char *file_name){
    int size;
    __asm__ volatile("int $0x80" : "=a"(size) : "a"(18), "b"(file_name));
    return size;
}
uint8_t get_scanecode(void){
    int scanecode;
    __asm__ volatile("int $0x80" : "=a"(scanecode) : "a"(19));
    return scanecode;
}
int write_file(char *filename, uint8_t *buffer, uint32_t size){
    int error;
    __asm__ volatile("int $0x80" : "=a"(error) : "a"(20), "b"(filename), "c"(buffer), "d"(size));
    return error;
}
unsigned int scancode_to_ascii(uint8_t scancode){
    unsigned int c;
    __asm__ volatile("int $0x80" : "=a"(c) : "a"(21), "b"(scancode));
    return c;
}
void memcpy(void* dest, void* src, int size) {
    uint8_t* d = (uint8_t*)dest;
    uint8_t* s = (uint8_t*)src;
    for(int i = 0; i < size; i++) { d[i] = s[i]; }
}
int getScreenWidth(void){
    int width;
    __asm__ volatile("int $0x80" : "=a"(width) : "a"(22));
    return width;
}
int getScreenHeight(void){
    int height;
    __asm__ volatile("int $0x80" : "=a"(height) : "a"(23));
    return height;
}
void draw_rect_filled(Rect *rect){
    __asm__ volatile("int $0x80" : : "a"(25), "b"(rect));
}
int get_screen_width(void){
    int w;
    __asm__ volatile("int $0x80" : "=a"(w) : "a"(26));
    return w;
}
int get_screen_height(void){
    int h;
    __asm__ volatile("int $0x80" : "=a"(h) : "a"(27));
    return h;
}
void draw_window(Window *win){
    __asm__ volatile("int $0x80" : : "a"(28), "b"(win));
}
void set_current_active_window(Window *win){
    __asm__ volatile("int $0x80" : : "a"(29), "b"(win));
}
Window *create_window(uint32_t bg_color){
    Window* win;
    __asm__ volatile("int $0x80" : "=a"(win) : "a"(30), "b"(bg_color));
    return win;
}
void close_window(Window *win){
    __asm__ volatile("int $0x80" : : "a"(31), "b"(win));
}
static void itoa_lib(int n, char* buffer, int base) {
    int i = 0;
    int isNeg = 0;
    if (n == 0) { buffer[0] = '0'; buffer[1] = '\0'; return; }
    if (n < 0 && base == 10) { isNeg = 1; n = -n; }

    while (n != 0) {
        int rem = n % base;
        buffer[i++] = (rem > 9) ? (rem - 10) + 'A' : rem + '0';
        n = n / base;
    }
    if (isNeg) buffer[i++] = '-';
    buffer[i] = '\0';

    int start = 0, end = i - 1;
    while (start < end) {
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;
        start++; end--;
    }
}
static void itoa_unsigned_lib(unsigned int n, char* buffer, int base) {
    int i = 0;
    if (n == 0) { buffer[0] = '0'; buffer[1] = '\0'; return; }
    while (n != 0) {
        unsigned int rem = n % base;
        buffer[i++] = (rem > 9) ? (rem - 10) + 'A' : rem + '0';
        n = n / base;
    }
    buffer[i] = '\0';
    int start = 0, end = i - 1;
    while (start < end) {
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;
        start++; end--;
    }
}
const char* utf8_to_unicode(const char* s, unsigned int* code) {
    unsigned char c = (unsigned char)*s;
    if (c < 0x80) { 
        *code = c;
        return s + 1;
    } else if ((c & 0xE0) == 0xC0) { 
        *code = ((c & 0x1F) << 6) | ((unsigned char)s[1] & 0x3F);
        return s + 2;
    } else if ((c & 0xF0) == 0xE0) {
        *code = ((c & 0x0F) << 12) | (((unsigned char)s[1] & 0x3F) << 6) | ((unsigned char)s[2] & 0x3F);
        return s + 3;
    }
    *code = c;
    return s + 1;
}
void vsprintf(unsigned int *str, const char *format, va_list args) {
    char temp[64];
    while (*format) {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 's': {
                    char *s = va_arg(args, char *);
                    if (!s) s = "(null)";
                    while (*s) *str++ = *s++;
                    break;
                }
                case 'd':
                case 'i': {
                    int n = va_arg(args, int);
                    itoa_lib(n, temp, 10);
                    char *t = temp;
                    while (*t) *str++ = *t++;
                    break;
                }
                case 'x': {
                    unsigned int n = va_arg(args, unsigned int);
                    itoa_unsigned_lib(n, temp, 16);
                    char *t = temp;
                    while (*t) *str++ = *t++;
                    break;
                }
                case 'c': {
                    char c = (unsigned int)va_arg(args, int);
                    *str++ = c;
                    break;
                }
                default:
                    *str++ = '%';
                    *str++ = *format;
                    break;
            }
        } else {
            unsigned int code;
            format = utf8_to_unicode(format, &code);
            *str++ = code;
            continue;
        }
        format++;
    }
    *str = '\0';
}
void print_unicode(unsigned int *str) {
    char out[1024]; 
    int pos = 0;
    for(int i = 0; str[i] != 0 && pos < 1020; i++){
        unsigned int c = str[i];
        if (c < 0x80) { out[pos++] = c; }
        else if (c < 0x800) { out[pos++] = 0xC0 | (c >> 6); out[pos++] = 0x80 | (c & 0x3F); }
        else { out[pos++] = 0xE0 | (c >> 12); out[pos++] = 0x80 | ((c >> 6) & 0x3F); out[pos++] = 0x80 | (c & 0x3F); }
    }
    out[pos] = '\0';
    write_file(stdout, (uint8_t*)out, pos);
}
void printf(const char* format, ...) {
    unsigned int buffer[256];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    print_unicode(buffer);
}
int strcmp(const char *c1, const char *c2) {
    while (*c1 && (*c1 == *c2)) {
        c1++;
        c2++;
    }
    return *(const unsigned char*)c1 - *(const unsigned char*)c2;
}
void print_window(Window *win, text_struct* ts){
    __asm__ volatile("int $0x80" : : "a"(35), "b"(win), "c"(ts));
}
void sleep(unsigned long ms){
    __asm__ volatile("int $0x80" : : "a"(36), "b"(ms));
}
int fork(process_struct *p){
    int pid;
    __asm__ volatile("int $0x80" : "=a"(pid) : "a"(37), "b"(p));
    return pid;
}
void kill(int pid){
    __asm__ volatile("int $0x80" : : "a"(38), "b"(pid));
}
void window_refresh(Window *win) {
    __asm__ volatile("int $0x80" : : "a"(39), "b"(win));
}
void window_redraw_content(Window *win) {
    __asm__ volatile("int $0x80" : : "a"(40), "b"(win));
}
void window_draw_char(Window *win, text_struct *ts, unsigned int c){
    __asm__ volatile("int $0x80" : : "a"(41), "b"(win), "c"(ts), "d"(c));
}
void system(char *cmd){
    __asm__ volatile(
        "int $0x80"
        :
        : "a"(42), "b"(cmd)
    );
}
void get_system_info(uint32_t *used, uint32_t *total, uint32_t *cpu) {
    __asm__ volatile("int $0x80" : : "a"(43), "b"(used), "c"(total), "d"(cpu));
}

int get_task_list(task_info_t *buffer, int max_tasks) {
    int count;
    __asm__ volatile("int $0x80" : "=a"(count) : "a"(44), "b"(buffer), "c"(max_tasks));
    return count;
}