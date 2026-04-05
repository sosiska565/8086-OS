#include "syscalls/syscalls.h"
#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "mm/memory.h"
#include "utils/utils.h"
#include "drivers/timer/timer.h"
#include "task/task.h"
#include "drivers/video/bga/gfx_console.h"
#include "fs/vfs.h"

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
    
    // if (current_task->uid == 0) { current_task->uid = regs->ebx; regs->eax = 0; }
    current_task->uid = regs->ebx; regs->eax = 0;
    // else regs->eax = -1; 
}


static void sys_read_file(struct syscall_registers *regs) { char abs_path[256]; get_absolute_path(current_task->cwd, (char*)regs->ebx, abs_path); regs->eax = vfs_read(abs_path, (uint8_t*)regs->ecx); }
static void sys_get_file_size(struct syscall_registers *regs) { char abs_path[256]; get_absolute_path(current_task->cwd, (char*)regs->ebx, abs_path); regs->eax = vfs_get_size(abs_path); }

extern void _set_console_color(unsigned int color);
static void sys_set_color(struct syscall_registers *regs) { _set_console_color(regs->ebx); set_background_color(regs->ecx); }
extern void gfx_get_cursor(int *x, int *y); extern void gfx_set_cursor(int x, int y);
static void sys_get_cursor(struct syscall_registers *regs) { gfx_get_cursor((int*)regs->ebx, (int*)regs->ecx); }
static void sys_set_cursor(struct syscall_registers *regs) { gfx_set_cursor((int)regs->ebx, (int)regs->ecx); }
static void sys_clear(struct syscall_registers *regs) { clear_screen(); regs->eax = 0; }

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

static syscall_handler_t syscall_table[256] = {
    [1]  = sys_exit, [2]  = sys_fork, [3]  = sys_read, [4]  = sys_write, [5]  = sys_read_file, [6]  = sys_write_file, 
    [7]  = sys_waitpid, [8]  = sys_get_file_size, [10] = sys_delete_file, [11] = sys_spawn, [12] = sys_chdir, [13] = sys_set_color,     
    [14] = sys_get_tasks, [15] = sys_get_mem_info, [16] = sys_get_cursor, [17] = sys_set_cursor, [18] = sys_clear, [19] = sys_getenv,
    [20] = sys_getuid, [21] = sys_setuid, 
    [37] = sys_kill, [39] = sys_mkdir, [45] = sys_malloc, [46] = sys_free, [79] = sys_getcwd, [89] = sys_readdir,   
};

void syscall_handler_c(struct syscall_registers *regs) {
    if (regs->eax < 256 && syscall_table[regs->eax] != NULL) syscall_table[regs->eax](regs); 
    else printf("Kernel Warning: Unhandled syscall ID %d\n", regs->eax);
}