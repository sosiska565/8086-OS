#include "system_apps/memory_viewer/memory_viewer.h"
#include "drivers/video/graphics.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "task/task.h"
#include "utils/utils.h"
#include "drivers/vga/vga.h"

static Window *mv_win;
static uint32_t current_address = 0x100000;
static int mv_running = 1;

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


static void win_print_hex32(Window *win, int x, int y, uint32_t val, uint32_t color) {
    const char *hex = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--) {
        window_draw_char(win, x + i * 8, y, hex[val & 0x0F], color);
        val >>= 4;
    }
}


static int get_mv_bytes_per_row(int win_width) {
    
    int bpr = (win_width - 84 - 16) / 32;
    
    if (bpr >= 16) return 16;
    if (bpr >= 8) return 8;
    if (bpr >= 4) return 4;
    if (bpr >= 2) return 2;
    return 1;
}

static void mem_viewer_draw() {
    while(mv_running) {
        draw_rect_filled(mv_win->x, mv_win->y, mv_win->width, mv_win->height, VGA32_COLOR_BLACK);
        draw_rect_filled(mv_win->x, mv_win->y, mv_win->width, 16, 0x000000AA);

        win_print_str(mv_win, 4, 4, " MEMORY VIEWER - Addr: 0x", VGA32_COLOR_WHITE);
        win_print_hex32(mv_win, 4 + 25 * 8, 4, current_address, VGA32_COLOR_YELLOW);

        int start_y = 24;
        int max_rows = (mv_win->height - 40) / 8;
        int bytes_per_row = get_mv_bytes_per_row(mv_win->width);

        for(int i = 0; i < max_rows; i++) {
            int y = start_y + i * 8;
            uint32_t row_addr = current_address + i * bytes_per_row;
            
            win_print_hex32(mv_win, 4, y, row_addr, VGA32_COLOR_YELLOW);

            
            
            if ((row_addr + bytes_per_row) <= 0x20200000) {
                uint8_t *ptr = (uint8_t*)row_addr;
                int ascii_start_x = 84 + bytes_per_row * 24 + 16;
                
                for(int j = 0; j < bytes_per_row; j++) {
                    win_print_hex(mv_win, 84 + j * 24, y, ptr[j], VGA32_COLOR_LIGHT_GREY);
                    
                    unsigned int c = ptr[j];
                    if(c < 32 || c > 126) c = '.';
                    window_draw_char(mv_win, ascii_start_x + j * 8, y, c, VGA32_COLOR_LIGHT_GREEN);
                }
            } else {
                win_print_str(mv_win, 84, y, "--- UNMAPPED MEMORY (PAGE FAULT PREVENTED) ---", VGA32_COLOR_RED);
            }
        }
        
        win_print_str(mv_win, 4, mv_win->height - 12, "ESC:Exit | ARROWS:Scroll | W/S:Fast | H:Heap | K:Kernel", VGA32_COLOR_WHITE);
        
        wm_render_window(mv_win);
        task_sleep(50);
    }
    exit_process();
}

static int mem_viewer_main(void) {
    mv_win = wm_create_window(VGA32_COLOR_BLACK);
    current_task->window = mv_win;
    current_task->owns_window = 1;
    wm_set_focused_window(mv_win);
    mv_running = 1;
    current_address = 0x100000;

    int draw_pid = create_process((void (*)(int, char**))mem_viewer_draw, 0, 0, "mv_draw", kernel_dir);

    while(1) {
        uint8_t sc = wait_scancode();
        if (sc & 0x80) continue;

        int bpr = get_mv_bytes_per_row(mv_win->width);

        if (sc == 0x01) { 
            mv_running = 0;
            break;
        } else if (sc == 0x48) { 
            current_address -= bpr;
        } else if (sc == 0x50) { 
            current_address += bpr;
        } else if (sc == 0x11) { 
            current_address -= bpr * 16;
        } else if (sc == 0x1F) { 
            current_address += bpr * 16;
        } else if (sc == 0x23) { 
            current_address = 0x00200000;
        } else if (sc == 0x25) { 
            current_address = 0x00100000;
        }
    }

    kill_task(draw_pid);
    return 0;
}

memory_viewer_t memoryViewer = {
    .main = mem_viewer_main
};