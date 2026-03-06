#include "utils/utils.h"
#include "drivers/vga/vga.h"
#include "drivers/timer/timer.h"
#include "mm/memory.h"
#include "fs/fat/fat32.h"
#include "global.h"
#include "drivers/video/vesa.h"
#include "drivers/video/graphics.h"
#include "task/task.h"

static unsigned long next = 1;

char sys_log_buffer[65536];
int sys_log_pos = 0;

void itoa_hex_32(uint32_t val, char *buf) {
    buf[0] = '0'; 
    buf[1] = 'x';
    const char* hex_chars = "0123456789ABCDEF";
    for(int i = 7; i >= 0; i--) {
        buf[2 + i] = hex_chars[val & 0xF];
        val >>= 4;
    }
    buf[10] = '\0';
}

void _print_screen(char *str, int x, int y, uint32_t color, uint32_t bg_color){
    while (*str) {
        unsigned int code;
        str = utf8_to_unicode(str, &code);
        vesa_draw_char(x, y, code, color, bg_color);
        x += 8;
    }
}

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
    printf("%s", err);
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

int get_pixels_in_string(char *str){
    int pixels = strlen(str) * 8;
    return pixels;
}

void klog(char *msg) {
    int i = 0;
    while(msg[i] != '\0' && sys_log_pos < 65534) {
        sys_log_buffer[sys_log_pos++] = msg[i++];
    }
    if(sys_log_pos < 65534) {
        sys_log_buffer[sys_log_pos++] = '\n';
    }
    sys_log_buffer[sys_log_pos] = '\0';
}

void klog_save() {
    if (isReadMode == 1) {
        printf("Cannot save log in Read-Only Mode!\n");
        return;
    }
    
    int result = fat32_write_file("sys.log", (uint8_t*)sys_log_buffer, sys_log_pos);
    if (result > 0) {
        printf("System log saved to sys.log (%d bytes)\n", sys_log_pos);
    } else {
        printf("Failed to save sys.log! Error: %d\n", result);
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
    }
    *code = c;
    return s + 1;
}

void panic_with_regs(registers_t *regs, char *msg) {
    __asm__ volatile("cli");

    uint32_t bg = VGA32_COLOR_BLUE; 
    uint32_t fg_text = VGA32_COLOR_WHITE;
    uint32_t fg_hex = VGA32_COLOR_YELLOW;
    uint32_t fg_alert = VGA32_COLOR_RED;

    clear_screen_vesa(bg);
    set_current_output_window(0);
    if(current_task) current_task->window = 0;

    int x_start = 16;
    int y = 16;
    int step = 16;
    char buf[16];

    _print_screen("  *** FATAL SYSTEM ERROR: KERNEL PANIC *** ", x_start, y, VGA32_COLOR_WHITE, bg);
    y += step * 2;

    _print_screen("MESSAGE: ", x_start, y, fg_text, bg);
    _print_screen(msg, x_start + (9 * 8), y, fg_hex, bg);
    y += step * 2;

    _print_screen("--- CPU REGISTERS ---", x_start, y, fg_text, bg);
    y += step;

    int col1 = x_start;
    int col2 = x_start + 160;
    int col3 = x_start + 320;
    int col4 = x_start + 480;

    itoa_hex_32(regs->eax, buf); _print_screen("EAX:", col1, y, fg_text, bg); _print_screen(buf, col1+40, y, fg_hex, bg);
    itoa_hex_32(regs->ebx, buf); _print_screen("EBX:", col2, y, fg_text, bg); _print_screen(buf, col2+40, y, fg_hex, bg);
    itoa_hex_32(regs->ecx, buf); _print_screen("ECX:", col3, y, fg_text, bg); _print_screen(buf, col3+40, y, fg_hex, bg);
    itoa_hex_32(regs->edx, buf); _print_screen("EDX:", col4, y, fg_text, bg); _print_screen(buf, col4+40, y, fg_hex, bg);
    y += step;

    itoa_hex_32(regs->esi, buf); _print_screen("ESI:", col1, y, fg_text, bg); _print_screen(buf, col1+40, y, fg_hex, bg);
    itoa_hex_32(regs->edi, buf); _print_screen("EDI:", col2, y, fg_text, bg); _print_screen(buf, col2+40, y, fg_hex, bg);
    itoa_hex_32(regs->ebp, buf); _print_screen("EBP:", col3, y, fg_text, bg); _print_screen(buf, col3+40, y, fg_hex, bg);
    itoa_hex_32(regs->esp, buf); _print_screen("ESP:", col4, y, fg_text, bg); _print_screen(buf, col4+40, y, fg_hex, bg);
    y += step * 2;

    _print_screen("--- EXECUTION CONTEXT ---", x_start, y, fg_text, bg);
    y += step;

    itoa_hex_32(regs->eip, buf); _print_screen("EIP (Instr Ptr): ", col1, y, fg_text, bg); _print_screen(buf, col1+140, y, fg_hex, bg);
    itoa_hex_32(regs->cs, buf);  _print_screen("CS (Code Seg):   ", col3, y, fg_text, bg); _print_screen(buf, col3+140, y, fg_hex, bg);
    y += step;

    itoa_hex_32(regs->eflags, buf); _print_screen("EFLAGS:          ", col1, y, fg_text, bg); _print_screen(buf, col1+140, y, fg_hex, bg);
    
    if (regs->int_no == 14) {
        uint32_t cr2;
        __asm__ volatile("mov %%cr2, %0" : "=r" (cr2));
        itoa_hex_32(cr2, buf);
        _print_screen("CR2 (Fault Addr):", col3, y, fg_text, bg); _print_screen(buf, col3+140, y, VGA32_COLOR_RED, bg);
    }
    y += step * 2;

    _print_screen("--- SYSTEM TASK ---", x_start, y, fg_text, bg);
    y += step;
    
    if (current_task) {
        _print_screen("FAULTING PROCESS: ", col1, y, fg_text, bg);
        _print_screen(current_task->name, col1 + 144, y, VGA32_COLOR_YELLOW, bg);
    } else {
        _print_screen("FAULTING PROCESS: KERNEL / INIT", col1, y, fg_text, bg);
    }

    y += step * 3;
    _print_screen("System halted. Please take a picture of this screen and reboot.", x_start, y, fg_text, bg);

    vesa_render_buffer();
    
    while(1){
        __asm__ volatile("hlt");
    }
}