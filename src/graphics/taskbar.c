#include "graphics/taskbar.h"
#include "drivers/video/graphics.h"
#include "drivers/video/vesa.h"
#include "drivers/vga/vga.h"
#include "multitask/task.h"
#include "drivers/rtc/rtc.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "utils/utils.h"
#include "memory/memory.h"
#include "global.h"

void safe_strncpy(char *dest, const char *src, int n) {
    int i;
    for (i = 0; i < n - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

void format_padded_num(int num, char* buff) {
    if (num < 10) {
        buff[0] = '0';
        itoa(num, buff + 1, 10);
    } else {
        itoa(num, buff, 10);
    }
}

void draw_taskbar(int argc, char **argv) {
    char buff[32];
    char task_name_buffer[32];
    char ram_str[32];
    char prc_str[16];
    
    uint32_t border_color  = 0x2A2A3A;
    uint32_t color_accent  = 0x00FFFF;
    uint32_t color_text    = 0xE0E0E0;
    uint32_t color_dimmed  = 0x666677;
    uint32_t color_active  = 0xFFDD00;
    uint32_t color_lang    = 0x00FFCC;
    uint32_t color_prc     = 0xFF00FF;
    
    while(1) {
        struct time t = rtc_get_time();
        int sw = get_screen_width();
        
        draw_rect_filled(0, 0, sw, 8, taskbar_color);
        draw_rect_filled(0, 7, sw, 1, border_color); 

        __asm__ volatile("cli");
        
        int task_count = 0;
        if (ready_queue) {
            Task *curr = ready_queue;
            do {
                if (curr->state != TASK_DEAD) task_count++;
                curr = curr->next;
            } while (curr != ready_queue);
        }

        Task *active_task = get_task_by_window(focused_window);
        if (active_task != 0 && active_task->name != 0) {
            safe_strncpy(task_name_buffer, active_task->name, 31);
        } else {
            safe_strncpy(task_name_buffer, "Desktop", 31);
        }
        __asm__ volatile("sti");

        size_t used_ram_mb = get_used_memory() / (1024 * 1024);
        size_t total_ram_mb = get_total_memory() / (1024 * 1024);
        
        uint32_t color_ram = 0x00FF00;
        if (used_ram_mb > total_ram_mb / 2) color_ram = 0xFFFF00;
        if (used_ram_mb > (total_ram_mb * 9) / 10) color_ram = 0xFF0000;

        ram_str[0] = '\0';
        itoa(used_ram_mb, buff, 10);
        strcat(ram_str, buff);
        strcat(ram_str, "/");
        itoa(total_ram_mb, buff, 10);
        strcat(ram_str, buff);
        strcat(ram_str, "MB");

        itoa(task_count, prc_str, 10);

        int lx = 4;
        
        _print_screen("RAM:", lx, 0, color_dimmed, taskbar_color); lx += 4 * 8;
        _print_screen(ram_str, lx, 0, color_ram, taskbar_color); lx += strlen(ram_str) * 8;
        _print_screen(" | ", lx, 0, color_dimmed, taskbar_color); lx += 3 * 8;

        _print_screen("PRC:", lx, 0, color_dimmed, taskbar_color); lx += 4 * 8;
        _print_screen(prc_str, lx, 0, color_prc, taskbar_color);

        _print_screen(" | WS:", lx, 0, color_dimmed, taskbar_color); lx += 6 * 8;
        itoa(current_workspace + 1, buff, 10);
        _print_screen(buff, lx, 0, color_accent, taskbar_color); lx += 2 * 8;
        
        _print_screen("GRID:", lx, 0, color_dimmed, taskbar_color); lx += 5 * 8;
        itoa(max_grid_cols, buff, 10);
        _print_screen(buff, lx, 0, color_active, taskbar_color);

        int task_str_size = strlen(task_name_buffer);
        int cx = (sw - ((task_str_size + 4) * 8)) / 2;
        
        _print_screen("[ ", cx, 0, color_dimmed, taskbar_color);
        _print_screen(task_name_buffer, cx + 16, 0, color_active, taskbar_color);
        _print_screen(" ]", cx + 16 + (task_str_size * 8), 0, color_dimmed, taskbar_color);

        int rx = sw - 4;
        
        format_padded_num(t.second, buff);
        rx -= 2 * 8;
        _print_screen(buff, rx, 0, color_dimmed, taskbar_color);
        
        rx -= 8;
        _print_screen(":", rx, 0, color_dimmed, taskbar_color);
        
        format_padded_num(t.minute, buff);
        rx -= 2 * 8;
        _print_screen(buff, rx, 0, color_text, taskbar_color);
        
        rx -= 8;
        _print_screen(":", rx, 0, color_dimmed, taskbar_color);
        
        format_padded_num(t.hour, buff);
        rx -= 2 * 8;
        _print_screen(buff, rx, 0, color_text, taskbar_color);

        rx -= 3 * 8;
        _print_screen(" | ", rx, 0, color_dimmed, taskbar_color);

        itoa(t.year, buff, 10);
        rx -= strlen(buff) * 8;
        _print_screen(buff, rx, 0, color_text, taskbar_color);
        
        rx -= 8; _print_screen(".", rx, 0, color_dimmed, taskbar_color);
        
        format_padded_num(t.month, buff);
        rx -= 2 * 8;
        _print_screen(buff, rx, 0, color_text, taskbar_color);
        
        rx -= 8; _print_screen(".", rx, 0, color_dimmed, taskbar_color);
        
        format_padded_num(t.day, buff);
        rx -= 2 * 8;
        _print_screen(buff, rx, 0, color_text, taskbar_color);

        rx -= 3 * 8;
        _print_screen(" | ", rx, 0, color_dimmed, taskbar_color);

        char *layout_str = current_layout == 0 ? "ENG" : "РУ";
        rx -= strlen(layout_str) * 8;
        _print_screen(layout_str, rx, 0, color_lang, taskbar_color);

        vesa_render_rect(0, 0, sw, 8);

        task_sleep(100);
    }
}