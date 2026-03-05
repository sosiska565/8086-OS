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
#include "programs/system/console/system.h"
#include "fs/vfs.h"
#include "utils/sysconfig.h"

extern int screen_width;
extern int screen_height;
extern int current_workspace;

typedef struct { int x; int y; int width; int height; uint32_t color; } Rect;
typedef void (*syscall_handler_t)(struct syscall_registers *);


static int is_task_visible() {
    if (!current_task || !current_task->window) return 1;
    return current_task->window->workspace == current_workspace;
}

static void sys_exit(struct syscall_registers *regs) { keyboard_flush(); exit_process(); }
static void sys_sleep(struct syscall_registers *regs) { task_sleep((int)regs->ebx); }
static void sys_fork(struct syscall_registers *regs) {
    process_struct *p = (process_struct*)regs->ebx;
    regs->eax = create_process((void (*)(int, char**))p->foo, p->argc, (char**)p->argv, (char*)p->name, current_task->page_dir);
}
static void sys_kill(struct syscall_registers *regs) { kill_task(regs->ebx); }
static void sys_wait_process(struct syscall_registers *regs) { wait_process((int)regs->ebx); }
static void sys_system_cmd(struct syscall_registers *regs) {
    char **tokens = parse_str((char*)regs->ebx, ' ');
    if(!tokens[0]) return;
    for(int i = 0; commands[i].name != NULL; i++) {
        if(strcmp(tokens[0], commands[i].name) == 0) { commands[i].handler(tokens); return; }
    }
}
static void sys_get_task_list(struct syscall_registers *regs) {
    task_info_t *buf = (task_info_t*)regs->ebx;
    int max = (int)regs->ecx, count = 0;
    __asm__ volatile("cli");
    if (ready_queue) {
        Task *t = ready_queue;
        do {
            if (t->state != TASK_DEAD && count < max) {
                buf[count].id = t->id; buf[count].parent_id = t->parent_id; buf[count].state = (int)t->state;
                int i = 0; while(t->name[i] && i < 31) { buf[count].name[i] = t->name[i]; i++; }
                buf[count].name[i] = '\0'; count++;
            }
            t = t->next;
        } while (t != ready_queue && count < max);
    }
    __asm__ volatile("sti");
    regs->eax = count;
}

static void sys_malloc(struct syscall_registers *regs) {
    void *ptr = kmalloc((size_t)regs->ebx);
    regs->eax = (uint32_t)ptr;
    track_allocation(current_task, ptr);
}
static void sys_free(struct syscall_registers *regs) {
    void *ptr = (void*)regs->ebx;
    untrack_allocation(current_task, ptr);
    kfree(ptr);
}
static void sys_get_sys_info(struct syscall_registers *regs) {
    if(regs->ebx) *(uint32_t*)regs->ebx = get_used_memory();
    if(regs->ecx) *(uint32_t*)regs->ecx = get_total_memory();
    if(regs->edx) *(uint32_t*)regs->edx = get_cpu_usage();
}
static void sys_random(struct syscall_registers *regs) { regs->eax = random(); }

static void sys_read_file(struct syscall_registers *regs) { regs->eax = (int)vfs_read((char*)regs->ebx, (uint8_t*)regs->ecx); }
static void sys_write_file(struct syscall_registers *regs) { regs->eax = vfs_write((char*)regs->ebx, (uint8_t*)regs->ecx, (uint32_t)regs->edx); }
static void sys_get_file_size(struct syscall_registers *regs) { regs->eax = (int)vfs_get_size((char*)regs->ebx); }

static void sys_getch(struct syscall_registers *regs) { regs->eax = (uint32_t)getch(); }
static void sys_gets(struct syscall_registers *regs) { gets((char*)regs->ebx, (int)regs->ecx); }
static void sys_wait_scancode(struct syscall_registers *regs) { regs->eax = wait_scancode(); }
static void sys_scancode_to_char(struct syscall_registers *regs) { regs->eax = scancode_to_char_layout((uint8_t)regs->ebx); }
static void sys_kbd_flush(struct syscall_registers *regs) { keyboard_flush(); }
static void sys_clear_screen(struct syscall_registers *regs) { clear_screen(); }
static void sys_print_color(struct syscall_registers *regs) { print_char_colored((char)regs->ebx, (uint8_t)regs->ecx); }
static void sys_print_num(struct syscall_registers *regs) { printnumber((int)regs->ebx); }
static void sys_print_hex(struct syscall_registers *regs) { printhex((unsigned int)regs->ebx); }
static void sys_vprintf(struct syscall_registers *regs) { vprintf((const char*)regs->ebx, *(va_list*)&regs->ecx); }

static void sys_win_putc(struct syscall_registers *regs) { window_putc(current_task->window, (unsigned int)regs->ebx); }
static void sys_draw_rect(struct syscall_registers *regs) {
    Rect *r = (Rect*)regs->ebx;
    if (current_task && current_task->window) window_draw_rect_filled(current_task->window, r->x, r->y, r->width, r->height, r->color);
    else draw_rect_filled(r->x, r->y, r->width, r->height, r->color);
}
static void sys_create_win(struct syscall_registers *regs) {
    Window *win = wm_create_window((uint32_t)regs->ebx);
    if (current_task) { current_task->window = win; current_task->owns_window = 1; }
    regs->eax = (uint32_t)win;
}
static void sys_close_win(struct syscall_registers *regs) { wm_close_window((Window*)regs->ebx); }
static void sys_win_print(struct syscall_registers *regs) {
    text_struct *ts = (text_struct*)regs->ecx;
    window_print((Window*)regs->ebx, ts->x, ts->y, ts->str, ts->color);
}
static void sys_win_render(struct syscall_registers *regs) { wm_render_window((Window*)regs->ebx); }
static void sys_win_redraw(struct syscall_registers *regs) { window_redraw_content((Window*)regs->ebx); }
static void sys_win_draw_char(struct syscall_registers *regs) {
    text_struct *ts = (text_struct*)regs->ecx;
    window_draw_char((Window*)regs->ebx, ts->x, ts->y, (unsigned int)regs->edx, ts->color);
}
static void sys_get_screen_w(struct syscall_registers *regs) { regs->eax = get_screen_width(); }
static void sys_get_screen_h(struct syscall_registers *regs) { regs->eax = get_screen_height(); }

static void sys_reload_config(struct syscall_registers *regs) { sysconfig_reload(); }

static syscall_handler_t syscall_table[256] = {[0]  = sys_win_putc,
    [1]  = sys_exit,
    [2]  = sys_getch,
    [3]  = sys_gets,
    [4]  = sys_malloc,
    [5]  = sys_free,
    [6]  = sys_clear_screen,
    [7]  = sys_print_color,
    [8]  = sys_random,
    [9]  = sys_print_num,
    [10] = sys_read_file,
    // [11..16] - Legacy VGA
    [17] = sys_print_hex,
    [18] = sys_get_file_size,[19] = sys_wait_scancode,
    [20] = sys_write_file,
    [21] = sys_scancode_to_char,
    [22] = sys_get_screen_w,
    [23] = sys_get_screen_h,
    [24] = sys_vprintf,
    [25] = sys_draw_rect,
    [26] = sys_get_screen_w,
    [27] = sys_get_screen_h,
    // [28, 29] - Legacy Window Calls
    [30] = sys_create_win,
    [31] = sys_close_win,
    [32] = sys_exit, 
    [33] = sys_kbd_flush,
    [34] = sys_wait_process,
    [35] = sys_win_print,
    [36] = sys_sleep,
    [37] = sys_fork,
    [38] = sys_kill,
    [39] = sys_win_render,
    [40] = sys_win_redraw,
    [41] = sys_win_draw_char,[42] = sys_system_cmd,
    [43] = sys_get_sys_info,
    [44] = sys_get_task_list,
    [45] = sys_reload_config
};

void syscall_handler_c(struct syscall_registers *regs) {
    if (regs->eax < 256 && syscall_table[regs->eax] != NULL) {
        syscall_table[regs->eax](regs); 
    } else {
        printf("Kernel Warning: Unhandled syscall ID %d\n", regs->eax);
    }
}