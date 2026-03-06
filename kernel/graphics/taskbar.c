#include "graphics/taskbar.h"
#include "drivers/video/graphics.h"
#include "drivers/video/vesa.h"
#include "drivers/vga/vga.h"
#include "task/task.h"
#include "drivers/rtc/rtc.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "utils/utils.h"
#include "mm/memory.h"
#include "global.h"

#define PILL_BG       0x001E1E2E
#define PILL_ACTIVE   0x00313244
#define TEXT_NORMAL   0x00CDD6F4
#define TEXT_ACCENT   0x0089B4FA
#define TEXT_GREEN    0x00A6E3A1
#define TEXT_RED      0x00F38BA8

void safe_strncpy(char *dest, const char *src, int n) {
    int i; for (i = 0; i < n - 1 && src[i] != '\0'; i++) dest[i] = src[i]; dest[i] = '\0';
}
void format_padded_num(int num, char* buff) {
    if (num < 10) { buff[0] = '0'; itoa(num, buff + 1, 10); } else itoa(num, buff, 10);
}

int draw_pill(int x, int y, char* text, uint32_t text_color, uint32_t bg_color) {
    int text_w = strlen(text) * 8;
    int pill_w = text_w + 24; 
    int pill_h = 24;
    draw_rounded_rect_filled(x, y, pill_w, pill_h, 12, bg_color);
    _print_screen(text, x + 12, y + 8, text_color, bg_color);
    return pill_w;
}

int taskbar_needs_update = 1;
static uint32_t *cached_taskbar_bg = 0;

void draw_taskbar(int argc, char **argv) {
    char buff[32]; char task_name_buffer[32]; 
    extern uint32_t *wallpaper_buf;
    extern int screen_pitch;
    
    while(1) {
        struct time t = rtc_get_time();
        int sw = get_screen_width();
        
        
        if (taskbar_needs_update || !cached_taskbar_bg) {
            if (cached_taskbar_bg) kfree(cached_taskbar_bg);
            cached_taskbar_bg = kmalloc(sw * TASKBAR_HEIGHT * 4);
            
            if (wallpaper_buf) fast_memcpy(back_buffer, wallpaper_buf, TASKBAR_HEIGHT * screen_pitch);
            else draw_rect_filled(0, 0, sw, TASKBAR_HEIGHT, DESKTOP_BG);
            
            apply_blur(0, 0, sw, TASKBAR_HEIGHT, taskbar_color);
            
            int bpp = screen_bpp / 8;
            for(int y=0; y<TASKBAR_HEIGHT; y++) {
                uint8_t *src = (uint8_t*)back_buffer + y * screen_pitch;
                for(int x=0; x<sw; x++) {
                    if (bpp == 4) cached_taskbar_bg[y*sw+x] = ((uint32_t*)src)[x];
                    else cached_taskbar_bg[y*sw+x] = src[x*3] | (src[x*3+1]<<8) | (src[x*3+2]<<16);
                }
            }
            taskbar_needs_update = 0;
        }

        
        int bpp = screen_bpp / 8;
        for(int y=0; y<TASKBAR_HEIGHT; y++) {
            uint8_t *dst = (uint8_t*)back_buffer + y * screen_pitch;
            for(int x=0; x<sw; x++) {
                uint32_t col = cached_taskbar_bg[y*sw+x];
                if (bpp == 4) ((uint32_t*)dst)[x] = col;
                else { dst[x*3] = col&0xFF; dst[x*3+1] = (col>>8)&0xFF; dst[x*3+2] = (col>>16)&0xFF; }
            }
        }

        __asm__ volatile("cli");
        int task_count = 0;
        if (ready_queue) {
            Task *curr = ready_queue;
            do { if (curr->state != TASK_DEAD) task_count++; curr = curr->next; } while (curr != ready_queue);
        }
        Task *active_task = get_task_by_window(focused_window);
        if (active_task && active_task->name) safe_strncpy(task_name_buffer, active_task->name, 31);
        else safe_strncpy(task_name_buffer, "Desktop", 31);
        __asm__ volatile("sti");
        
        size_t used_ram_mb = get_used_memory() / (1024 * 1024);
        char stats_buf[64];
        strcpy(stats_buf, "RAM: "); itoa(used_ram_mb, buff, 10); strcat(stats_buf, buff);
        strcat(stats_buf, "MB | PROCS: "); itoa(task_count, buff, 10); strcat(stats_buf, buff);
        
        char time_buf[32];
        format_padded_num(t.hour, buff); strcpy(time_buf, buff); strcat(time_buf, ":");
        format_padded_num(t.minute, buff); strcat(time_buf, buff); strcat(time_buf, ":");
        format_padded_num(t.second, buff); strcat(time_buf, buff);

        int curr_x = 10;
        int py = 4; 
        
        curr_x += draw_pill(curr_x, py, stats_buf, TEXT_GREEN, PILL_BG) + 10;
        curr_x += draw_pill(curr_x, py, time_buf, TEXT_ACCENT, PILL_BG) + 10;
        
        int ws_pill_w = 4 * 24 + 8;
        int ws_pill_x = sw - ws_pill_w - 10;
        draw_rounded_rect_filled(ws_pill_x, py, ws_pill_w, 24, 12, PILL_BG);
        for(int i=0; i<4; i++) {
            char ws_buf[2] = { '1' + i, '\0' };
            int tx = ws_pill_x + 4 + i * 24 + 8;
            if (i == current_workspace) {
                draw_rounded_rect_filled(ws_pill_x + 4 + i * 24, py + 2, 20, 20, 10, PILL_ACTIVE);
                _print_screen(ws_buf, tx, py + 8, TEXT_ACCENT, PILL_ACTIVE);
            } else {
                _print_screen(ws_buf, tx, py + 8, TEXT_NORMAL, PILL_BG);
            }
        }
        
        int title_w = strlen(task_name_buffer) * 8 + 24;
        int title_x = ws_pill_x - title_w - 10;
        draw_pill(title_x, py, task_name_buffer, TEXT_RED, PILL_BG);

        vesa_render_rect(0, 0, sw, TASKBAR_HEIGHT);
        
        task_sleep(100);
    }
}