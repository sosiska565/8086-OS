#include "syscalls/syscalls.h"
#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "mm/memory.h"
#include "utils/utils.h"
#include "drivers/timer/timer.h"
#include "task/task.h"
#include "drivers/video/bga/gfx_console.h"
#include "fs/vfs.h"
#include "drivers/mouse/mouse.h"
#include "fs/fd.h"

typedef void (*syscall_handler_t)(struct syscall_registers *);

static void sys_exit(struct syscall_registers *regs) { keyboard_flush(); exit_process(); }

static void sys_read(struct syscall_registers *regs) {
    int fd = (int)regs->ebx; char* buf = (char*)regs->ecx;
    if (fd == 0) { buf[0] = (char)getch(); regs->eax = 1; return; }
    regs->eax = -1; 
}

static void sys_write(struct syscall_registers *regs) {
    int fd = (int)regs->ebx; char* buf = (char*)regs->ecx; uint32_t size = (uint32_t)regs->edx;
    if (fd == 1 || fd == 2) { 
        if (current_task->redirect_buf != 0) {
            for(uint32_t i = 0; i < size; i++) {
                if (current_task->redirect_size < 65535) { 
                    current_task->redirect_buf[current_task->redirect_size++] = buf[i];
                }
            }
            regs->eax = size; return;
        }

        uint32_t i = 0;
        while (i < size) {
            unsigned int code = 0;
            const char* next = utf8_to_unicode(&buf[i], &code);
            int bytes_read = next - &buf[i];
            if (bytes_read <= 0 || i + bytes_read > size) gfx_putc(buf[i++]);
            else { gfx_putc(code); i += bytes_read; }
        }
        regs->eax = size; return;
    }
    regs->eax = -1; 
}

static void sys_write_file(struct syscall_registers *regs) {
    char abs_path[256]; get_absolute_path(current_task->cwd, (char*)regs->ebx, abs_path);
    if (current_task->uid != 0) {
        if (strcmp(abs_path, "/kernel.cfg") == 0 || strncmp(abs_path, "/path", 5) == 0) { regs->eax = -1; return; }
    }
    regs->eax = vfs_write(abs_path, (uint8_t*)regs->ecx, (uint32_t)regs->edx);
}

static void sys_delete_file(struct syscall_registers *regs) {
    char abs_path[256]; get_absolute_path(current_task->cwd, (char*)regs->ebx, abs_path);
    if (current_task->uid != 0) { 
        if (strcmp(abs_path, "/kernel.cfg") == 0 || strncmp(abs_path, "/path", 5) == 0) { regs->eax = -1; return; }
    }
    regs->eax = vfs_delete(abs_path);
}

static void sys_spawn(struct syscall_registers *regs) {
    char abs_path[256]; get_absolute_path(current_task->cwd, (char*)regs->ebx, abs_path);
    char* redirect = (char*)regs->edx;
    if (redirect && redirect[0] != '\0') {
        char r_path[256]; get_absolute_path(current_task->cwd, redirect, r_path);
        regs->eax = spawn_process(abs_path, (char**)regs->ecx, r_path);
    } else {
        regs->eax = spawn_process(abs_path, (char**)regs->ecx, NULL);
    }
}

static void sys_getuid(struct syscall_registers *regs) { regs->eax = current_task->uid; }
static void sys_setuid(struct syscall_registers *regs) {
    current_task->uid = regs->ebx; regs->eax = 0;
}

static void sys_read_file(struct syscall_registers *regs) { char abs_path[256]; get_absolute_path(current_task->cwd, (char*)regs->ebx, abs_path); regs->eax = vfs_read(abs_path, (uint8_t*)regs->ecx); }
static void sys_get_file_size(struct syscall_registers *regs) { char abs_path[256]; get_absolute_path(current_task->cwd, (char*)regs->ebx, abs_path); regs->eax = vfs_get_size(abs_path); }

extern void _set_console_color(unsigned int color);
static void sys_set_color(struct syscall_registers *regs) { _set_console_color(regs->ebx); set_background_color(regs->ecx); }
extern void gfx_get_cursor(int *x, int *y); extern void gfx_set_cursor(int x, int y);
static void sys_get_cursor(struct syscall_registers *regs) { gfx_get_cursor((int*)regs->ebx, (int*)regs->ecx); }
static void sys_set_cursor(struct syscall_registers *regs) { gfx_set_cursor((int)regs->ebx, (int)regs->ecx); }
static void sys_clear(struct syscall_registers *regs) { clear_screen(); regs->eax = 0; }

extern int term_cols;
extern int term_rows;
static void sys_get_term_size(struct syscall_registers *regs) {
    int *cols = (int*)regs->ebx;
    int *rows = (int*)regs->ecx;
    if (cols) *cols = term_cols;
    if (rows) *rows = term_rows;
    regs->eax = 0;
}

static void sys_print(struct syscall_registers *regs) {
    char* str = (char*)regs->ebx;
    if (!str) { regs->eax = 0; return; }
    int i = 0;
    while (str[i] != '\0') { gfx_putc(str[i]); i++; }
    regs->eax = i; 
}

typedef struct { int id; int parent_id; int state; char name[32]; } task_info_user_t;
static void sys_get_tasks(struct syscall_registers *regs) {
    task_info_user_t* buf = (task_info_user_t*)regs->ebx; int max_count = (int)regs->ecx; int count = 0;
    if (!ready_queue) { regs->eax = 0; return; }
    Task *t = ready_queue;
    do { if (t->state != TASK_DEAD && count < max_count) { buf[count].id = t->id; buf[count].parent_id = t->parent_id; buf[count].state = t->state; strcpy(buf[count].name, t->name); count++; } t = t->next; } while (t != ready_queue); regs->eax = count;
}
static void sys_get_mem_info(struct syscall_registers *regs) { uint32_t* used = (uint32_t*)regs->ebx; uint32_t* total = (uint32_t*)regs->ecx; *used = get_used_memory(); *total = get_total_memory(); }

extern Config *global_cfg;
static void sys_getenv(struct syscall_registers *regs) {
    char* key = (char*)regs->ebx; char* out = (char*)regs->ecx;
    if (!global_cfg) { regs->eax = 0; return; }
    char* val = config_get_value(global_cfg, key);
    if (val) { strcpy(out, val); regs->eax = 1; } else regs->eax = 0;
}

static void sys_fork(struct syscall_registers *regs) { process_struct *p = (process_struct*)regs->ebx; regs->eax = create_process((void (*)(int, char**))p->foo, p->argc, (char**)p->argv, (char*)p->name, current_task->page_dir); }
static void sys_waitpid(struct syscall_registers *regs) { wait_process((int)regs->ebx); }
static void sys_kill(struct syscall_registers *regs) { kill_task(regs->ebx); }
static void sys_malloc(struct syscall_registers *regs) { void *ptr = kmalloc((size_t)regs->ebx); regs->eax = (uint32_t)ptr; track_allocation(current_task, ptr); }
static void sys_free(struct syscall_registers *regs) { void *ptr = (void*)regs->ebx; untrack_allocation(current_task, ptr); kfree(ptr); }
static void sys_chdir(struct syscall_registers *regs) { char abs_path[256]; get_absolute_path(current_task->cwd, (char*)regs->ebx, abs_path); uint8_t type; if (vfs_get_attr(abs_path, &type) && type == VFS_ATTR_DIR) { strcpy(current_task->cwd, abs_path); regs->eax = 0; } else regs->eax = -1; }
static void sys_readdir(struct syscall_registers *regs) { char abs_path[256]; get_absolute_path(current_task->cwd, (char*)regs->ebx, abs_path); regs->eax = vfs_readdir(abs_path, (int)regs->ecx, (vfs_dirent_t*)regs->edx); }
static void sys_mkdir(struct syscall_registers *regs) { char abs_path[256]; get_absolute_path(current_task->cwd, (char*)regs->ebx, abs_path); regs->eax = vfs_mkdir(abs_path); }
static void sys_getcwd(struct syscall_registers *regs) { strcpy((char*)regs->ebx, current_task->cwd); regs->eax = 0; }
static void sys_dlopen(struct syscall_registers *regs) { regs->eax = load_library(current_task, (char*)regs->ebx); }
static void sys_dlsym(struct syscall_registers *regs) { regs->eax = get_symbol(current_task, regs->ebx, (char*)regs->ecx); }


static void sys_mount(struct syscall_registers *regs) { regs->eax = vfs_mount((char*)regs->ebx, (char*)regs->ecx, (char*)regs->edx); }
static void sys_unmount(struct syscall_registers *regs) { regs->eax = vfs_unmount((char*)regs->ebx); }

static void sys_detach(struct syscall_registers *regs) {
    current_task->parent_id = 1; 
    regs->eax = 0;
}

extern int screen_width;
extern int screen_height;
extern int screen_bpp;
extern int screen_pitch;

static void sys_get_screen_info(struct syscall_registers *regs) {
    int *w = (int*)regs->ebx;
    int *h = (int*)regs->ecx;
    int *bpp = (int*)regs->edx;
    
    if(w) *w = screen_width;
    if(h) *h = screen_height;
    if(bpp) *bpp = screen_bpp;
    regs->eax = 0;
}

extern uint32_t *video_memory; 

static void sys_flush_screen(struct syscall_registers *regs) {
    uint32_t *user_buffer = (uint32_t*)regs->ebx;
    
    if (user_buffer && video_memory) {
        fast_memcpy(video_memory, user_buffer, screen_height * screen_pitch);
    }
    regs->eax = 0;
}

static void sys_get_mouse(struct syscall_registers *regs) {
    int *x = (int*)regs->ebx;
    int *y = (int*)regs->ecx;
    int *buttons = (int*)regs->edx; 

    if(x) *x = mouse.x;
    if(y) *y = mouse.y;
    if(buttons) {
        *buttons = 0;
        if(mouse.left_button) *buttons |= 1;
        if(mouse.right_button) *buttons |= 2;
        if(mouse.middle_button) *buttons |= 4;
    }
    regs->eax = 0;
}

static void sys_poll_key(struct syscall_registers *regs) {
    regs->eax = poll_buffer(); 
}

static void sys_get_key_modifiers(struct syscall_registers *regs) {
    regs->eax = get_keyboard_modifiers();
}

static void sys_yield(struct syscall_registers *regs) {
    yield();
}


static void sys_sbrk(struct syscall_registers *regs) {
    int incr = (int)regs->ebx;
    uint32_t old_end = current_task->heap_end;
    
    if (incr == 0) {
        regs->eax = old_end;
        return;
    }

    uint32_t new_end = old_end + incr;
    uint32_t start_page = (old_end + 4095) & ~4095;
    uint32_t end_page = (new_end + 4095) & ~4095;

    for (uint32_t page = start_page; page < end_page; page += 4096) {
        uint32_t phys = (uint32_t)kmalloc_a(4096);
        if (!phys) {
            regs->eax = -1;
            return;
        }
        fast_memset((void*)phys, 0, 1024); 
        paging_map_user(current_task->page_dir, phys, page, 7);
        track_allocation_a(current_task, (void*)phys);
    }

    current_task->heap_end = new_end;
    regs->eax = old_end;
}

static void wrap_open(struct syscall_registers *regs) { regs->eax = sys_open((char*)regs->ebx, regs->ecx); }
static void wrap_close(struct syscall_registers *regs) { regs->eax = sys_close(regs->ebx); }
static void wrap_read_fd(struct syscall_registers *regs) { 
    if (regs->ebx == 0) { 
        char* buf = (char*)regs->ecx; buf[0] = (char)getch(); regs->eax = 1; return;
    }
    regs->eax = sys_read_fd(regs->ebx, (uint8_t*)regs->ecx, regs->edx); 
}
static void wrap_write_fd(struct syscall_registers *regs) {
    if (regs->ebx == 1 || regs->ebx == 2) { sys_write(regs); return; } 
    regs->eax = sys_write_fd(regs->ebx, (uint8_t*)regs->ecx, regs->edx);
}
static void wrap_lseek(struct syscall_registers *regs) { regs->eax = sys_lseek(regs->ebx, regs->ecx, regs->edx); }

static void sys_get_ticks(struct syscall_registers *regs) {
    regs->eax = (uint32_t)get_ticks();
}

extern int get_key_state(uint8_t scancode);
static void sys_get_key_state(struct syscall_registers *regs) {
    regs->eax = get_key_state((uint8_t)regs->ebx);
}

static syscall_handler_t syscall_table[256] = {
    [1]  = sys_exit, [2]  = sys_fork, [3]  = sys_read, [4]  = sys_write, [5]  = sys_read_file, [6]  = sys_write_file, 
    [7]  = sys_waitpid, [8]  = sys_get_file_size, [10] = sys_delete_file, [11] = sys_spawn, [12] = sys_chdir, [13] = sys_set_color,     
    [14] = sys_get_tasks, [15] = sys_get_mem_info, [16] = sys_get_cursor, [17] = sys_set_cursor, [18] = sys_clear, [19] = sys_getenv,
    [20] = sys_getuid, [21] = sys_setuid, [22] = sys_get_term_size, [23] = sys_print,
    [37] = sys_kill, [39] = sys_mkdir, [45] = sys_malloc, [46] = sys_free, [79] = sys_getcwd, [89] = sys_readdir,   
    [90] = sys_dlopen, [91] = sys_dlsym,
    [92] = sys_mount, [93] = sys_unmount,
    [24] = sys_detach,
    [25] = sys_get_screen_info, 
    [26] = sys_flush_screen, 
    [27] = sys_get_mouse, 
    [28] = sys_poll_key,
    [29] = sys_get_key_modifiers,
    [30] = sys_yield,
    [31] = sys_get_ticks,
    [32] = sys_get_key_state,
    
    [100] = wrap_open,
    [101] = wrap_close,
    [102] = wrap_lseek,
    [103] = sys_sbrk,
    [104] = wrap_read_fd,
    [105] = wrap_write_fd
};

void syscall_handler_c(struct syscall_registers *regs) {
    if (regs->eax < 256 && syscall_table[regs->eax] != NULL) syscall_table[regs->eax](regs); 
    else printf("Kernel Warning: Unhandled syscall ID %d\n", regs->eax);
}