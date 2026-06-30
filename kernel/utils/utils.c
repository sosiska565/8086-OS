/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/kernel/utils/utils.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include "utils/utils.h"
#include "drivers/vga/vga.h"
#include "drivers/timer/timer.h"
#include "mm/memory.h"
#include "fs/vfs.h"      
#include "global.h"
#include "drivers/video/vesa.h"
#include "task/task.h"
#include "kernel/include/string.h"

#define KLOG_MAX_SIZE 64000
#define KLOG_TRIM_SIZE 16000 

static unsigned long next = 1;

char sys_log_buffer[65536];
int sys_log_pos = 0;

void itoa_hex_32(uint32_t val, char *buf) {
    buf[0] = '0'; buf[1] = 'x';
    const char* hex_chars = "0123456789ABCDEF";
    for(int i = 7; i >= 0; i--) { buf[2 + i] = hex_chars[val & 0xF]; val >>= 4; }
    buf[10] = '\0';
}

void vsprintf(char *str, const char *format, va_list args) {
    int pos = 0;
    while (*format) {
        if (*format == '%') {
            format++;
            if (*format == 'd' || *format == 'i') {
                int n = va_arg(args, int);
                char buf[16]; int i = 0, is_neg = 0;
                if (n == 0) { str[pos++] = '0'; }
                else {
                    if (n < 0) { is_neg = 1; n = -n; }
                    while (n > 0) { buf[i++] = (n % 10) + '0'; n /= 10; }
                    if (is_neg) buf[i++] = '-';
                    while (--i >= 0) str[pos++] = buf[i];
                }
            } else if (*format == 'u') {
                uint32_t n = va_arg(args, uint32_t);
                char buf[16]; int i = 0;
                if (n == 0) { str[pos++] = '0'; }
                else {
                    while (n > 0) { buf[i++] = (n % 10) + '0'; n /= 10; }
                    while (--i >= 0) str[pos++] = buf[i];
                }
            } else if (*format == 'x' || *format == 'X') {
                unsigned int n = va_arg(args, unsigned int);
                char buf[16]; int i = 0;
                if (n == 0) { str[pos++] = '0'; }
                else {
                    while (n > 0) { int rem = n % 16; buf[i++] = (rem < 10) ? (rem + '0') : (rem - 10 + 'A'); n /= 16; }
                    while (--i >= 0) str[pos++] = buf[i];
                }
            } else if (*format == 'c') {
                str[pos++] = (char)va_arg(args, int);
            } else if (*format == 's') {
                char* s = va_arg(args, char*);
                if (!s) s = "(null)";
                while (*s) str[pos++] = *s++;
            } else if (*format == '%') {
                str[pos++] = '%';
            }
        } else { str[pos++] = *format; }
        format++;
    }
    str[pos] = '\0';
}

void sprintf(char *str, const char *format, ...) { 
    va_list args; 
    va_start(args, format); 
    vsprintf(str, format, args); 
    va_end(args); 
}

void _print_screen(char *str, int x, int y, uint32_t color, uint32_t bg_color){
    while (*str) {
        unsigned int code;
        str = utf8_to_unicode(str, &code);
        vesa_draw_char(x, y, code, color, bg_color);
        x += 8;
    }
}

void srand(unsigned long seed){ next = seed; }
unsigned long random(void){
    unsigned long m = 1UL << 31; 
    next = (1664525 * next + 1013904223) % m;
    return next;
}

void get_cpu_vendor(char *buffer) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    buffer[0] = (ebx & 0xFF); buffer[1] = (ebx >> 8) & 0xFF; buffer[2] = (ebx >> 16) & 0xFF; buffer[3] = (ebx >> 24) & 0xFF;
    buffer[4] = (edx & 0xFF); buffer[5] = (edx >> 8) & 0xFF; buffer[6] = (edx >> 16) & 0xFF; buffer[7] = (edx >> 24) & 0xFF;
    buffer[8] = (ecx & 0xFF); buffer[9] = (ecx >> 8) & 0xFF; buffer[10] = (ecx >> 16) & 0xFF; buffer[11] = (ecx >> 24) & 0xFF;
    buffer[12] = '\0';
}

void panic(char *err){
    set_text_color(4);
    clear_screen();
    printf("\nKernel panic!\nErr: %s\nSystem will reboot in 5 seconds...\n", err);
    
    vesa_render_buffer(); 
    
    __asm__ volatile ("sti"); 
    unsigned long newTick = get_ticks() + 5000;
    while(get_ticks() < newTick) {
        __asm__ volatile("hlt"); 
    }
    __asm__ volatile ("mov $0xFE, %al\nout %al, $0x64\n");
}

int is_space(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

char* trim_whitespace(char* str) {
    while(is_space(*str)) str++;
    if(*str == 0) return str;
    char* end = str + strlen(str) - 1;
    while(end > str && is_space(*end)) { *end = '\0'; end--; }
    return str;
}

Config* config_parse(char* buffer) {
    Config* cfg = (Config*)kmalloc(sizeof(Config));
    int max_entries = 64; 
    cfg->entries = (ConfigEntry*)kmalloc(sizeof(ConfigEntry) * max_entries);
    cfg->count = 0; cfg->capacity = max_entries;
    char *line_start = buffer, *line_end;
    while (*line_start) {
        char *ptr = line_start;
        while (*ptr && *ptr != '\n') ptr++;
        line_end = ptr; int is_last_line = (*ptr == '\0'); *line_end = '\0'; 
        char *eq_sign = line_start;
        while (*eq_sign && *eq_sign != '=') eq_sign++;
        if (*eq_sign == '=') {
            *eq_sign = '\0';
            char *key = trim_whitespace(line_start);
            char *val = trim_whitespace(eq_sign + 1);
            if (key[0] != '#' && key[0] != '\0' && cfg->count < cfg->capacity) {
                cfg->entries[cfg->count].key = key;
                cfg->entries[cfg->count].value = val;
                cfg->count++;
            }
        }
        if (is_last_line) break;
        line_start = line_end + 1;
    }
    return cfg;
}

char* config_get_value(Config* cfg, const char* key) {
    if (!cfg) return 0;
    for (int i = 0; i < cfg->count; i++) if (strcmp(cfg->entries[i].key, key) == 0) return cfg->entries[i].value;
    return 0;
}

void config_free(Config* cfg) {
    if (cfg) { if (cfg->entries) kfree(cfg->entries); kfree(cfg); }
}

void config_set_value(Config *cfg, char *key, char *new_val) {
    for (int i = 0; i < cfg->count; i++) {
        if (strcmp(cfg->entries[i].key, key) == 0) { cfg->entries[i].value = new_val; return; }
    }
}

void config_save(char *filename, Config *cfg) {
    uint8_t *out_buffer = (uint8_t*)kmalloc(4096);
    int pos = 0;
    for (int i = 0; i < cfg->count; i++) {
        char *k = cfg->entries[i].key; char *v = cfg->entries[i].value;
        while (*k) out_buffer[pos++] = *k++;
        out_buffer[pos++] = '=';
        while (*v) out_buffer[pos++] = *v++;
        out_buffer[pos++] = '\n';
    }
    
    vfs_write(filename, out_buffer, pos);
    
    kfree(out_buffer);
}

int get_pixels_in_string(char *str){ return strlen(str) * 8; }

void klog(char *msg) {
    char time_str[32];
    itoa(get_ticks() / 1000, time_str, 10);
    
    
    char formatted_msg[256];
    formatted_msg[0] = '\0';
    strcat(formatted_msg, "[");
    strcat(formatted_msg, time_str);
    strcat(formatted_msg, "s] ");
    strcat(formatted_msg, msg);
    strcat(formatted_msg, "\n");

    int msg_len = strlen(formatted_msg);

    
    if (sys_log_pos + msg_len >= KLOG_MAX_SIZE) {
        int shift_amount = sys_log_pos - KLOG_TRIM_SIZE;
        if (shift_amount > 0) {
            
            memmove(sys_log_buffer, sys_log_buffer + KLOG_TRIM_SIZE, shift_amount);
            sys_log_pos = shift_amount;
        } else {
            sys_log_pos = 0;
        }
    }
    
    
    for (int i = 0; i < msg_len; i++) {
        sys_log_buffer[sys_log_pos++] = formatted_msg[i];
    }
    sys_log_buffer[sys_log_pos] = '\0'; 
}


void klog_save() {
    int result = vfs_write("/sys.log", (uint8_t*)sys_log_buffer, sys_log_pos);
}

const char* utf8_to_unicode(const char* s, unsigned int* code) {
    unsigned char c = (unsigned char)*s;
    if (c < 0x80) { *code = c; return s + 1; } 
    else if ((c & 0xE0) == 0xC0) { *code = ((c & 0x1F) << 6) | ((unsigned char)s[1] & 0x3F); return s + 2; }
    else if ((c & 0xF0) == 0xE0) { 
        *code = ((c & 0x0F) << 12) | (((unsigned char)s[1] & 0x3F) << 6) | ((unsigned char)s[2] & 0x3F); 
        return s + 3; 
    }
    *code = c; return s + 1;
}

void panic_with_regs(registers_t *regs, char *msg) {
    __asm__ volatile("cli");
    clear_screen_vesa(VGA32_COLOR_BLUE);
    _print_screen("FATAL ERROR", 16, 16, VGA32_COLOR_WHITE, VGA32_COLOR_BLUE);
    while(1){ __asm__ volatile("hlt"); }
}

void get_absolute_path(char* cwd, char* input_path, char* output_path) {
    char temp[512];
    if (input_path[0] == '/') { strcpy(temp, input_path); } 
    else {
        strcpy(temp, cwd); int len = strlen(temp);
        if (len > 0 && temp[len-1] != '/') strcat(temp, "/");
        strcat(temp, input_path);
    }
    char* stack[64]; int top = 0; char* p = temp;
    while (*p) {
        while (*p == '/') p++; 
        if (!*p) break;
        char* start = p;
        while (*p && *p != '/') p++;
        if (*p) { *p = '\0'; p++; }
        
        if (strcmp(start, ".") == 0) continue;
        else if (strcmp(start, "..") == 0) { if (top > 0) top--; } 
        else { if (top < 64) stack[top++] = start; }
    }
    output_path[0] = '/'; output_path[1] = '\0';
    for (int i = 0; i < top; i++) {
        strcat(output_path, stack[i]);
        if (i < top - 1) strcat(output_path, "/");
    }
}
