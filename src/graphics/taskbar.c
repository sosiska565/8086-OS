#include "graphics/taskbar.h"
#include "drivers/video/graphics.h"
#include "drivers/video/vesa.h"
#include "drivers/vga/vga.h"
#include "multitask/task.h"
#include "drivers/rtc/rtc.h"

#define TASKBAR_COLOR 0x191970

void _print_screen(char *str, int x, int y, uint32_t color, uint32_t bg_color){
    for(int i = 0; str[i] != '\0'; i++){
        vesa_draw_char(x, y, str[i], color, bg_color);
        x += 8;
    }
}

void draw_taskbar(int argc, char **argv) {
    char buff[32];
    while(1) {
        struct time t = rtc_get_time();
        Task *active_task = get_task_by_window(focused_window);
        int task_str_size = strlen(active_task->name);
        
        draw_rect_filled(0, 0, get_screen_width(), 8, TASKBAR_COLOR);

        itoa(t.hour, buff);
        _print_screen(buff, 0, 0, VGA32_COLOR_WHITE, TASKBAR_COLOR);
        _print_screen(":", 16, 0, VGA32_COLOR_WHITE, TASKBAR_COLOR);

        itoa(t.minute, buff);
        _print_screen(buff, 24, 0, VGA32_COLOR_WHITE, TASKBAR_COLOR);
        _print_screen(":", 40, 0, VGA32_COLOR_WHITE, TASKBAR_COLOR);
        
        itoa(t.second, buff);
        _print_screen(buff, 48, 0, VGA32_COLOR_WHITE, TASKBAR_COLOR);
        _print_screen(
            active_task->name,
            (get_screen_width() - (task_str_size * 8)) /2,
            0,
            VGA32_COLOR_WHITE,
            TASKBAR_COLOR
        );
        task_sleep(500);
    }
}