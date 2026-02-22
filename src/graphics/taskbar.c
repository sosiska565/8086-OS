#include "graphics/taskbar.h"
#include "drivers/video/graphics.h"
#include "drivers/video/vesa.h"
#include "drivers/vga/vga.h"
#include "multitask/task.h"
#include "drivers/rtc/rtc.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "utils/utils.h"

#define TASKBAR_COLOR 0x191970

void safe_strncpy(char *dest, const char *src, int n) {
    int i;
    for (i = 0; i < n - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

void _print_screen(char *str, int x, int y, uint32_t color, uint32_t bg_color){
    while (*str) {
        unsigned int code;
        str = utf8_to_unicode(str, &code);
        vesa_draw_char(x, y, code, color, bg_color);
        x += 8;
    }
}

void draw_taskbar(int argc, char **argv) {
    char buff[32];
    char task_name_buffer[32];
    
    while(1) {
        struct time t = rtc_get_time();
        
        draw_rect_filled(0, 0, get_screen_width(), 8, TASKBAR_COLOR);

        itoa(t.hour, buff);
        _print_screen(buff, 0, 0, VGA32_COLOR_WHITE, TASKBAR_COLOR);
        _print_screen(":", 16, 0, VGA32_COLOR_WHITE, TASKBAR_COLOR);

        itoa(t.minute, buff);
        _print_screen(buff, 24, 0, VGA32_COLOR_WHITE, TASKBAR_COLOR);
        _print_screen(":", 40, 0, VGA32_COLOR_WHITE, TASKBAR_COLOR);
        
        itoa(t.second, buff);
        _print_screen(buff, 48, 0, VGA32_COLOR_WHITE, TASKBAR_COLOR);

        __asm__ volatile("cli");
        
        Task *active_task = get_task_by_window(focused_window);
        
        if (active_task != 0 && active_task->name != 0) {
            safe_strncpy(task_name_buffer, active_task->name, 31);
        } else {
            safe_strncpy(task_name_buffer, "Desktop", 31);
        }
        
        __asm__ volatile("sti");
        
        int task_str_size = strlen(task_name_buffer);
        
        _print_screen(
            task_name_buffer,
            (get_screen_width() - (task_str_size * 8)) / 2,
            0,
            VGA32_COLOR_WHITE,
            TASKBAR_COLOR
        );

        char date[32];
        date[0] = '\0';
        strcat(date, current_layout == 0 ? "ENG" : "РУ");
        strcat(date, " | ");
        itoa(t.day, buff);
        strcat(date, buff);
        strcat(date, "/");
        itoa(t.month, buff);
        strcat(date, buff);
        strcat(date, "/");
        itoa(t.year, buff);
        strcat(date, buff);
        _print_screen(date, get_screen_width() - (strlen(date) * 8), 0, VGA32_COLOR_WHITE, TASKBAR_COLOR);

        vesa_render_rect(0, 0, get_screen_width(), 8);

        task_sleep(100);
    }
}