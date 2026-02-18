#include "programs/system/syscalls/syscalls.h"
#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "memory/memory.h"
#include "utils/utils.h"
#include "drivers/timer/timer.h"
#include "fs/fat/fat32.h"
#include <stdarg.h>
#include "drivers/video/vesa.h"
#include "drivers/video/graphics.h"
#include "multitask/task.h"

extern int screen_width;
extern int screen_height;

typedef struct {
    int x;
    int y;
    int width;
    int height;
    uint32_t color;
} Rect;

void syscall_handler_c(struct registers *regs){
    if(regs->eax == 0) {
        window_putc(current_task->window, (char)regs->ebx);
    }
    else if(regs->eax == 1) {
        keyboard_flush();
        exit_process();
    }
    else if(regs->eax == 2) regs->eax = (uint32_t)getch();
    else if(regs->eax == 3) gets((char*)regs->ebx, (int)regs->ecx);
    else if(regs->eax == 4) {
        void *ptr = kmalloc((size_t)regs->ebx);
        regs->eax = (uint32_t)ptr;
        track_allocation(current_task, ptr);
    }
    else if(regs->eax == 5) {
        void *ptr = (void*)regs->ebx;
        untrack_allocation(current_task, ptr);
        kfree(ptr);
    }
    else if(regs->eax == 6) clear_screen();
    else if(regs->eax == 7) print_char_colored((char)regs->ebx, (uint8_t)regs->ecx);
    else if(regs->eax == 8) regs->eax = random();
    else if(regs->eax == 9) printnumber((int)regs->ebx);
    else if(regs->eax == 10) regs->eax = (int)fat32_read_file((char *)regs->ebx, (uint8_t*)regs->ecx);
    else if(regs->eax == 11) set_cursor_position((unsigned int)regs->ebx, (unsigned int)regs->ecx);
    else if(regs->eax == 12) set_background_color((vga_color_t)regs->ebx);
    else if(regs->eax == 13) set_text_color((vga_color_t)regs->ebx);
    else if(regs->eax == 14) get_cursor_xy((unsigned int *)&regs->ebx, (unsigned int *)&regs->ecx);
    else if(regs->eax == 15) regs->eax = (vga_color_t)get_current_color();
    else if(regs->eax == 16) set_current_color((vga_color_t)regs->ebx);
    else if(regs->eax == 17) printhex((unsigned int)regs->ebx);
    else if(regs->eax == 18) regs->eax = (int)fat32_get_file_size((char *)regs->ebx);
    else if(regs->eax == 19) regs->eax = wait_scancode();
    else if(regs->eax == 20) regs->eax = fat32_write_file((char*)regs->ebx, (uint8_t*)regs->ecx, (uint32_t)regs->edx);
    else if(regs->eax == 21) regs->eax = scancode_to_char((uint8_t)regs->ebx);
    else if(regs->eax == 22) regs->eax = screen_width;
    else if(regs->eax == 23) regs->eax = screen_height;
    else if(regs->eax == 24) vprintf((const char *)regs->ebx, *(va_list*)&regs->ecx);
    else if(regs->eax == 25) {
        Rect *rect = (Rect*)regs->ebx;
        draw_rect_filled(rect->x, rect->y, rect->width, rect->height, rect->color);
    }
    else if(regs->eax == 26) regs->eax = get_screen_width();
    else if(regs->eax == 27) regs->eax = get_screen_height();
    else if(regs->eax == 28) {
        Window *win = (Window*)regs->ebx;
        draw_window(win, win->x, win->y, win->width, win->height, VGA32_COLOR_CYAN, win->bg_color, 0);
    }
    else if(regs->eax == 29){
        Window *win = (Window*)regs->ebx;
        set_current_output_window(win);
    }
    else if(regs->eax == 30){
        Window *win = wm_create_window((uint32_t)regs->ebx);
        
        if (current_task) {
            current_task->window = win;
            current_task->owns_window = 1; 
        }
        
        regs->eax = (uint32_t)win;
    }
    else if(regs->eax == 31){
        wm_close_window((Window *)regs->ebx);
    }
    else if(regs->eax == 32) exit_process();
    else if(regs->eax == 33) keyboard_flush();
    else if(regs->eax == 34) wait_process((int)regs->ebx);
    else if(regs->eax == 35) {
        text_struct *ts = (text_struct*)regs->ecx;
        window_print((Window*)regs->ebx, ts->x, ts->y, ts->str, ts->color);
    }
    else if(regs->eax == 36) task_sleep((int)regs->ebx);
    else if(regs->eax == 37) {
        process_struct *p = (process_struct*)regs->ebx;
        regs->eax = create_process((void (*)(int, char**))p->foo, p->argc, (char**)p->argv, (char*)p->name);
    }
    else if(regs->eax == 38) kill_task(regs->ebx);
    else {
        printf("Unknown syscall!\n");
    }
}