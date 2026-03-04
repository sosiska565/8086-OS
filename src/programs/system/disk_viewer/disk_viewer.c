#include "programs/system/disk_viewer/disk_viewer.h"
#include "drivers/video/graphics.h"
#include "drivers/file/ATA/ATA.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "multitask/task.h"
#include "utils/utils.h"
#include "drivers/vga/vga.h"

static Window *dv_win;
static int current_sector = 0;
static uint8_t sector_buffer[512];
static int dv_running = 1;

static void win_print_str(Window *win, int x, int y, char *str, uint32_t color) {
    while(*str) {
        window_draw_char(win, x, y, *str++, color);
        x += 8;
    }
}


static void win_print_hex(Window *win, int x, int y, uint8_t val, uint32_t color) {
    const char *hex = "0123456789ABCDEF";
    window_draw_char(win, x, y, hex[(val >> 4) & 0x0F], color);
    window_draw_char(win, x + 8, y, hex[val & 0x0F], color);
}


static void win_print_offset16(Window *win, int x, int y, uint16_t val, uint32_t color) {
    const char *hex = "0123456789ABCDEF";
    for (int i = 3; i >= 0; i--) {
        window_draw_char(win, x + i * 8, y, hex[val & 0x0F], color);
        val >>= 4;
    }
}


static int get_dv_bytes_per_row(int win_width) {
    
    
    int bpr = (win_width - 68) / 32;
    
    if (bpr >= 16) return 16;
    if (bpr >= 8) return 8;
    if (bpr >= 4) return 4;
    if (bpr >= 2) return 2;
    return 1;
}

static void disk_viewer_draw() {
    while(dv_running) {
        
        draw_rect_filled(dv_win->x, dv_win->y, dv_win->width, dv_win->height, VGA32_COLOR_BLACK);
        
        draw_rect_filled(dv_win->x, dv_win->y, dv_win->width, 16, 0x000000AA);

        char header[64];
        strcpy(header, " Disk Viewer - Sector: ");
        char num_buf[16];
        itoa(current_sector, num_buf, 10);
        strcat(header, num_buf);
        win_print_str(dv_win, 4, 4, header, VGA32_COLOR_WHITE);

        int start_y = 24;
        int max_rows = (dv_win->height - 40) / 8;
        int bytes_per_row = get_dv_bytes_per_row(dv_win->width);

        for(int i = 0; i < max_rows; i++) {
            int y = start_y + i * 8;
            int offset = i * bytes_per_row;
            if (offset >= 512) break;

            win_print_offset16(dv_win, 4, y, offset, VGA32_COLOR_YELLOW);

            int ascii_start_x = 52 + bytes_per_row * 24 + 16;

            for(int j = 0; j < bytes_per_row; j++) {
                int idx = offset + j;
                if(idx < 512) {
                    win_print_hex(dv_win, 52 + j * 24, y, sector_buffer[idx], VGA32_COLOR_LIGHT_GREY);
                    
                    unsigned int c = sector_buffer[idx];
                    if(c < 32 || c > 126) c = '.';
                    window_draw_char(dv_win, ascii_start_x + j * 8, y, c, VGA32_COLOR_LIGHT_GREEN);
                }
            }
        }
        
        win_print_str(dv_win, 4, dv_win->height - 12, "ESC:Exit | ARROWS:Scroll | W/S:Fast", VGA32_COLOR_WHITE);
        
        wm_render_window(dv_win);
        task_sleep(50);
    }
    exit_process();
}

static int disk_viewer_main(void) {
    dv_win = wm_create_window(VGA32_COLOR_BLACK);
    current_task->window = dv_win;
    current_task->owns_window = 1;
    wm_set_focused_window(dv_win);
    dv_running = 1;
    current_sector = 0;

    disk_read_sector(current_sector, sector_buffer);

    int draw_pid = create_process((void (*)(int, char**))disk_viewer_draw, 0, 0, "dv_draw", kernel_dir);

    while(1) {
        uint8_t sc = wait_scancode();
        if (sc & 0x80) continue;

        if (sc == 0x01) { 
            dv_running = 0;
            break;
        } else if (sc == 0x48) { 
            if (current_sector > 0) current_sector--;
            disk_read_sector(current_sector, sector_buffer);
        } else if (sc == 0x50) { 
            current_sector++;
            disk_read_sector(current_sector, sector_buffer);
        } else if (sc == 0x11) { 
            if (current_sector >= 1500) current_sector -= 1500;
            else current_sector = 0;
            disk_read_sector(current_sector, sector_buffer);
        } else if (sc == 0x1F) { 
            current_sector += 1500;
            disk_read_sector(current_sector, sector_buffer);
        }
    }

    kill_task(draw_pid);
    return 0;
}

Disk_viewer disk_viewer = {
    .main = disk_viewer_main
};