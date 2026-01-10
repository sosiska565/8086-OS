#include "utils/utils.h"
#include "drivers/vga/vga.h"
#include "drivers/timer/timer.h"
#include "memory/memory.h"
#include "fs/fat/fat32.h"

static unsigned long next = 1;

void srand(unsigned long seed){
    next = seed;
}

unsigned long random(void){
    unsigned long m = 1UL << 31; 
    next = (1664525 * next + 1013904223) % m;
    return next;
}

void get_cpu_vendor(char *buffer) {
    uint32_t eax, ebx, ecx, edx;

    __asm__ volatile("cpuid" 
                     : "=b"(ebx), "=c"(ecx), "=d"(edx) 
                     : "a"(0));

    buffer[0] = (ebx & 0xFF);
    buffer[1] = (ebx >> 8) & 0xFF;
    buffer[2] = (ebx >> 16) & 0xFF;
    buffer[3] = (ebx >> 24) & 0xFF;
    buffer[4] = (edx & 0xFF);
    buffer[5] = (edx >> 8) & 0xFF;
    buffer[6] = (edx >> 16) & 0xFF;
    buffer[7] = (edx >> 24) & 0xFF;
    buffer[8] = (ecx & 0xFF);
    buffer[9] = (ecx >> 8) & 0xFF;
    buffer[10] = (ecx >> 16) & 0xFF;
    buffer[11] = (ecx >> 24) & 0xFF;
    buffer[12] = '\0';
}

void panic(char *err){
    set_text_color(4);
    clear_screen();
    printf("\nKernel panic!\n");
    printf("Err: ");
    printf("%d", err);
    printf("\nSystem will reboot in 5 seconds...\n");
    unsigned long newTick = get_ticks() + 9100;

    while(get_ticks() < newTick);

    __asm__ volatile (
        "mov $0xFE, %al\n"
        "out %al, $0x64\n"
    );
}

int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

char* trim_whitespace(char* str) {
    char* end;

    while(is_space(*str)) str++;

    if(*str == 0) return str;

    end = str + strlen(str) - 1;

    while(end > str && is_space(*end)) {
        *end = '\0';
        end--;
    }

    return str;
}

Config* config_parse(char* buffer) {
    Config* cfg = (Config*)kmalloc(sizeof(Config));
    
    int max_entries = 64; 
    cfg->entries = (ConfigEntry*)kmalloc(sizeof(ConfigEntry) * max_entries);
    cfg->count = 0;
    cfg->capacity = max_entries;

    char *line_start = buffer;
    char *line_end;

    while (*line_start) {
        char *ptr = line_start;
        while (*ptr && *ptr != '\n') ptr++;
        
        line_end = ptr;
        int is_last_line = (*ptr == '\0');
        *line_end = '\0'; 

        char *eq_sign = line_start;
        while (*eq_sign && *eq_sign != '=') eq_sign++;

        if (*eq_sign == '=') {
            *eq_sign = '\0';

            char *key = trim_whitespace(line_start);
            char *val = trim_whitespace(eq_sign + 1);

            if (key[0] != '#' && key[0] != '\0' && cfg->count < cfg->capacity) {
                int i = cfg->count;
                cfg->entries[i].key = key;
                cfg->entries[i].value = val;
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

    for (int i = 0; i < cfg->count; i++) {
        if (strcmp(cfg->entries[i].key, key) == 0) {
            return cfg->entries[i].value;
        }
    }
    return 0;
}

void config_free(Config* cfg) {
    if (cfg) {
        if (cfg->entries) kfree(cfg->entries);
        kfree(cfg);
    }
}

void config_set_value(Config *cfg, char *key, char *new_val) {
    for (int i = 0; i < cfg->count; i++) {
        if (strcmp(cfg->entries[i].key, key) == 0) {
            cfg->entries[i].value = new_val;
            return;
        }
    }
}

void config_save(char *filename, Config *cfg) {
    uint8_t *out_buffer = (uint8_t*)kmalloc(4096);
    int pos = 0;

    for (int i = 0; i < cfg->count; i++) {
        char *k = cfg->entries[i].key;
        char *v = cfg->entries[i].value;

        while (*k) out_buffer[pos++] = *k++;
        
        out_buffer[pos++] = ' ';
        out_buffer[pos++] = '=';
        out_buffer[pos++] = ' ';

        while (*v) out_buffer[pos++] = *v++;

        out_buffer[pos++] = '\n';
    }

    fat32_write_file(filename, out_buffer, pos);

    kfree(out_buffer);
}