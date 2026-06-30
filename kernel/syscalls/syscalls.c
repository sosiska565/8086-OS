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
#include "lwip/raw.h"
#include "lwip/ip.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "kernel/include/string.h"
#include "drivers/rtc/rtc.h"

typedef void (*syscall_handler_t)(struct syscall_registers *);

extern void vesa_render_buffer(void);

static void sys_exit(struct syscall_registers *regs) { keyboard_flush(); exit_process(); }
static void sys_read(struct syscall_registers *regs) { regs->eax = sys_read_fd((int)regs->ebx, (uint8_t*)regs->ecx, (uint32_t)regs->edx); }
static void sys_write(struct syscall_registers *regs) { 
    regs->eax = sys_write_fd((int)regs->ebx, (uint8_t*)regs->ecx, (uint32_t)regs->edx); 
}
static void sys_write_file(struct syscall_registers *regs) { char abs_path[256]; get_absolute_path(current_task->cwd, (char*)regs->ebx, abs_path); if (current_task->uid != 0) { if (strcmp(abs_path, "/kernel.cfg") == 0 || strncmp(abs_path, "/path", 5) == 0) { regs->eax = -1; return; } } regs->eax = vfs_write(abs_path, (uint8_t*)regs->ecx, (uint32_t)regs->edx); }
static void sys_delete_file(struct syscall_registers *regs) { char abs_path[256]; get_absolute_path(current_task->cwd, (char*)regs->ebx, abs_path); if (current_task->uid != 0) { if (strcmp(abs_path, "/kernel.cfg") == 0 || strncmp(abs_path, "/path", 5) == 0) { regs->eax = -1; return; } } regs->eax = vfs_delete(abs_path); }
static void sys_spawn(struct syscall_registers *regs) { char abs_path[256]; get_absolute_path(current_task->cwd, (char*)regs->ebx, abs_path); char* redirect = (char*)regs->edx; if (redirect && redirect[0] != '\0') { char r_path[256]; get_absolute_path(current_task->cwd, redirect, r_path); int fd = sys_open(r_path, 1 | 0x0200 | 0x0400); if(fd != -1) { regs->eax = spawn_process_ext(abs_path, (char**)regs->ecx, -1, fd); sys_close(fd); } else { regs->eax = -1; } } else { regs->eax = spawn_process(abs_path, (char**)regs->ecx); } }
static void sys_spawn_ext(struct syscall_registers *regs) { char abs_path[256]; get_absolute_path(current_task->cwd, (char*)regs->ebx, abs_path); regs->eax = spawn_process_ext(abs_path, (char**)regs->ecx, (int)regs->edx, (int)regs->esi); }
static void sys_getuid(struct syscall_registers *regs) { regs->eax = current_task->uid; }
static void sys_setuid(struct syscall_registers *regs) { current_task->uid = regs->ebx; regs->eax = 0; }
static void sys_read_file(struct syscall_registers *regs) { char abs_path[256]; get_absolute_path(current_task->cwd, (char*)regs->ebx, abs_path); regs->eax = vfs_read(abs_path, (uint8_t*)regs->ecx); }
static void sys_get_file_size(struct syscall_registers *regs) { char abs_path[256]; get_absolute_path(current_task->cwd, (char*)regs->ebx, abs_path); regs->eax = vfs_get_size(abs_path); }

extern void _set_console_color(unsigned int color);
static void sys_set_color(struct syscall_registers *regs) { _set_console_color(regs->ebx); set_background_color(regs->ecx); }
extern void gfx_get_cursor(int *x, int *y); extern void gfx_set_cursor(int x, int y);
static void sys_get_cursor(struct syscall_registers *regs) { gfx_get_cursor((int*)regs->ebx, (int*)regs->ecx); }
static void sys_set_cursor(struct syscall_registers *regs) { gfx_set_cursor((int)regs->ebx, (int)regs->ecx); }
static void sys_clear(struct syscall_registers *regs) { clear_screen(); regs->eax = 0; }

extern int term_cols; extern int term_rows;
static void sys_get_term_size(struct syscall_registers *regs) { int *cols = (int*)regs->ebx; int *rows = (int*)regs->ecx; if (cols) *cols = term_cols; if (rows) *rows = term_rows; regs->eax = 0; }
static void sys_print(struct syscall_registers *regs) { char* str = (char*)regs->ebx; if (!str) { regs->eax = 0; return; } int i = 0; while (str[i] != '\0') { gfx_putc(str[i]); i++; } regs->eax = i; }

typedef struct { int id; int parent_id; int state; char name[32]; } task_info_user_t;
static void sys_get_tasks(struct syscall_registers *regs) { task_info_user_t* buf = (task_info_user_t*)regs->ebx; int max_count = (int)regs->ecx; int count = 0; if (!ready_queue) { regs->eax = 0; return; } Task *t = ready_queue; do { if (t->state != TASK_DEAD && count < max_count) { buf[count].id = t->id; buf[count].parent_id = t->parent_id; buf[count].state = t->state; strcpy(buf[count].name, t->name); count++; } t = t->next; } while (t != ready_queue); regs->eax = count; }
static void sys_get_mem_info(struct syscall_registers *regs) { uint32_t* used = (uint32_t*)regs->ebx; uint32_t* total = (uint32_t*)regs->ecx; *used = get_used_memory(); *total = get_total_memory(); }

extern Config *global_cfg;
static void sys_getenv(struct syscall_registers *regs) { char* key = (char*)regs->ebx; char* out = (char*)regs->ecx; if (!global_cfg) { regs->eax = 0; return; } char* val = config_get_value(global_cfg, key); if (val) { strcpy(out, val); regs->eax = 1; } else regs->eax = 0; }

static void sys_fork(struct syscall_registers *regs) { process_struct *p = (process_struct*)regs->ebx; regs->eax = create_process((void (*)(int, char**))p->foo, p->argc, (char**)p->argv, (char*)p->name, current_task->page_dir, -1, -1); }
static void sys_waitpid(struct syscall_registers *regs) { wait_process((int)regs->ebx); }
extern void send_signal(int pid, int sig);
static void sys_kill(struct syscall_registers *regs) { 
    int sig = (regs->ecx == 0) ? 9 : regs->ecx; 
    send_signal(regs->ebx, sig); 
    regs->eax = 0;
}
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
    int target_pid = (int)regs->ebx;
    
    
    if (target_pid <= 0) {
        current_task->parent_id = 1; 
    } else {
        
        if (ready_queue) {
            Task *t = ready_queue;
            do {
                if (t->id == target_pid) { 
                    t->parent_id = 1; 
                    break; 
                }
                t = t->next;
            } while (t != ready_queue);
        }
    }
    regs->eax = 0; 
}
extern int screen_width; extern int screen_height; extern int screen_bpp; extern int screen_pitch;
static void sys_get_screen_info(struct syscall_registers *regs) { int *w = (int*)regs->ebx; int *h = (int*)regs->ecx; int *bpp = (int*)regs->edx; if(w) *w = screen_width; if(h) *h = screen_height; if(bpp) *bpp = screen_bpp; regs->eax = 0; }

extern uint32_t *video_memory; 
static void sys_flush_screen(struct syscall_registers *regs) { uint32_t *user_buffer = (uint32_t*)regs->ebx; if (user_buffer && video_memory) { fast_memcpy(video_memory, user_buffer, screen_height * screen_pitch); } regs->eax = 0; }
static void sys_get_mouse(struct syscall_registers *regs) { int *x = (int*)regs->ebx; int *y = (int*)regs->ecx; int *buttons = (int*)regs->edx; if(x) *x = mouse.x; if(y) *y = mouse.y; if(buttons) { *buttons = 0; if(mouse.left_button) *buttons |= 1; if(mouse.right_button) *buttons |= 2; if(mouse.middle_button) *buttons |= 4; } regs->eax = 0; }
static void sys_poll_key(struct syscall_registers *regs) { regs->eax = poll_buffer(); }
static void sys_get_key_modifiers(struct syscall_registers *regs) { regs->eax = get_keyboard_modifiers(); }
static void sys_yield(struct syscall_registers *regs) { yield(); }

static void sys_sbrk(struct syscall_registers *regs) {
    int incr = (int)regs->ebx; uint32_t old_end = current_task->heap_end;
    if (incr == 0) { regs->eax = old_end; return; }
    uint32_t new_end = old_end + incr;
    uint32_t start_page = (old_end + 4095) & ~4095;
    uint32_t end_page = (new_end + 4095) & ~4095;
    for (uint32_t page = start_page; page < end_page; page += 4096) {
        uint32_t phys = (uint32_t)kmalloc_a(4096);
        if (!phys) { regs->eax = -1; return; }
        fast_memset((void*)phys, 0, 1024); 
        paging_map_user(current_task->page_dir, phys, page, 7);
        track_allocation_a(current_task, (void*)phys);
    }
    current_task->heap_end = new_end; regs->eax = old_end;
}

static void wrap_open(struct syscall_registers *regs) { regs->eax = sys_open((char*)regs->ebx, regs->ecx); }
static void wrap_close(struct syscall_registers *regs) { regs->eax = sys_close(regs->ebx); }
static void wrap_read_fd(struct syscall_registers *regs) { regs->eax = sys_read_fd(regs->ebx, (uint8_t*)regs->ecx, regs->edx); }
static void wrap_write_fd(struct syscall_registers *regs) { regs->eax = sys_write_fd(regs->ebx, (uint8_t*)regs->ecx, regs->edx); }
static void wrap_lseek(struct syscall_registers *regs) { regs->eax = sys_lseek(regs->ebx, regs->ecx, regs->edx); }
static void sys_pipe(struct syscall_registers *regs) { regs->eax = sys_pipe_impl((int*)regs->ebx); }
static void sys_get_ticks(struct syscall_registers *regs) { regs->eax = (uint32_t)get_ticks(); }
extern int get_key_state(uint8_t scancode);
static void sys_get_key_state(struct syscall_registers *regs) { regs->eax = get_key_state((uint8_t)regs->ebx); }

#define MAX_SHM 32
struct { int key; uint32_t phys_addr; uint32_t size; int owner_pid; } shm_table[MAX_SHM] = {0};

static void sys_shm_get(struct syscall_registers *regs) {
    int key = regs->ebx; uint32_t size = regs->ecx;

    if (size == 0xFFFFFFFF) { 
        for (int i = 0; i < MAX_SHM; i++) {
            if (shm_table[i].key == key && shm_table[i].phys_addr) {
                regs->eax = 1; 
                return;
            }
        }
        regs->eax = 0; 
        return;
    }

    if (size == 0) { 
        for (int i = 0; i < MAX_SHM; i++) {
            if (shm_table[i].key == key && shm_table[i].phys_addr) {
                kfree_a((void*)shm_table[i].phys_addr);
                shm_table[i].phys_addr = 0;
                shm_table[i].key = 0;
                shm_table[i].size = 0;
                shm_table[i].owner_pid = 0;
                regs->eax = 0; return;
            }
        }
        regs->eax = -1; return;
    }

    for(int i=0; i<MAX_SHM; i++) if(shm_table[i].key == key && shm_table[i].phys_addr) { regs->eax = shm_table[i].size; return; }

    for(int i=0; i<MAX_SHM; i++) {
        if(shm_table[i].phys_addr == 0) {
            shm_table[i].key = key; 
            shm_table[i].size = (size + 4095) & ~4095; 
            shm_table[i].phys_addr = (uint32_t)kmalloc_a(shm_table[i].size);
            shm_table[i].owner_pid = current_task->id; 
            fast_memset((void*)shm_table[i].phys_addr, 0, shm_table[i].size/4);
            regs->eax = shm_table[i].size; return;
        }
    }
    regs->eax = -1;
}

static void sys_shm_map(struct syscall_registers *regs) {
    int key = regs->ebx;
    for(int i=0; i<MAX_SHM; i++) {
        if(shm_table[i].key == key && shm_table[i].phys_addr) {
            uint32_t vaddr = 0x80000000 + (i * 0x01000000); 
            for(uint32_t p = 0; p < shm_table[i].size; p += 4096) {
                paging_map_user(current_task->page_dir, shm_table[i].phys_addr + p, vaddr + p, 7);
            }
            regs->eax = vaddr; return;
        }
    }
    regs->eax = 0;
}



void net_close_socket(int pcb_id) { if (pcb_id) raw_remove((struct raw_pcb*)pcb_id); }
void net_close_tcp_socket(int pcb_id) {
    struct tcp_pcb *pcb = (struct tcp_pcb*)pcb_id;
    if (pcb) {
        tcp_arg(pcb, NULL);
        tcp_recv(pcb, NULL);
        tcp_err(pcb, NULL);
        tcp_poll(pcb, NULL, 0);
        tcp_sent(pcb, NULL);
        
        tcp_abort(pcb);
    }
}

static u8_t raw_net_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr) {
    int sys_fd = (int)arg;
    if (sys_fd >= 0 && sys_fd < MAX_OPEN_FILES) {
        KFile *kf = &system_open_files[sys_fd];
        if (kf->in_use && kf->type == FILE_TYPE_SOCKET && kf->buffer != NULL) {
            int len = p->tot_len;
            if (len > kf->capacity) len = kf->capacity;
            pbuf_copy_partial(p, kf->buffer, len, 0);
            kf->size = len;
            kf->is_dirty = 1; 
        }
    }

    pbuf_free(p);
    return 1; 
}

static err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    int sys_fd = (int)arg;
    if (p == NULL) { system_open_files[sys_fd].is_dirty = -1; return ERR_OK; } 
    
    KFile *kf = &system_open_files[sys_fd];
    if (kf->in_use && kf->buffer != NULL) {
        
        if (kf->size + p->tot_len > kf->capacity) {
            return ERR_MEM; 
        }
        
        pbuf_copy_partial(p, kf->buffer + kf->size, p->tot_len, 0);
        kf->size += p->tot_len;
        kf->is_dirty = 1;
        
    }
    pbuf_free(p);
    return ERR_OK;
}

static err_t tcp_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err) {
    int sys_fd = (int)arg;
    system_open_files[sys_fd].is_dirty = 1; 
    return ERR_OK;
}

static void sys_socket(struct syscall_registers *regs) {
    int type = (int)regs->ecx;
    int proto = (int)regs->edx;

    int free_fd = -1;
    for (int i = 3; i < MAX_FDS; i++) if (current_task->fd_table[i] == -1) { free_fd = i; break; }
    if (free_fd == -1) { regs->eax = -1; return; }

    int sys_fd = -1;
    for (int i = 0; i < MAX_OPEN_FILES; i++) if (!system_open_files[i].in_use) { sys_fd = i; break; }
    if (sys_fd == -1) { regs->eax = -1; return; }

    system_open_files[sys_fd].in_use = 1;
    system_open_files[sys_fd].refcount = 1;
    system_open_files[sys_fd].capacity = 65536; 
    system_open_files[sys_fd].buffer = kmalloc(65536);
    system_open_files[sys_fd].is_dirty = 0;
    system_open_files[sys_fd].size = 0;
    system_open_files[sys_fd].offset = 0;

    if (type == 3) { 
        struct raw_pcb *pcb = raw_new(proto);
        system_open_files[sys_fd].type = FILE_TYPE_SOCKET;
        system_open_files[sys_fd].socket_id = (int)pcb;
        raw_recv(pcb, raw_net_recv, (void*)sys_fd);
    } else if (type == 1) { 
        struct tcp_pcb *pcb = tcp_new();
        system_open_files[sys_fd].type = FILE_TYPE_SOCKET_TCP;
        system_open_files[sys_fd].socket_id = (int)pcb;
        tcp_arg(pcb, (void*)sys_fd);
        tcp_recv(pcb, tcp_recv_callback);
    } else {
        system_open_files[sys_fd].in_use = 0;
        kfree(system_open_files[sys_fd].buffer);
        regs->eax = -1; return;
    }

    current_task->fd_table[free_fd] = sys_fd;
    regs->eax = free_fd;
}

struct _sys_sockaddr_in { uint8_t sin_len; uint8_t sin_family; uint16_t sin_port; uint32_t s_addr; char sin_zero[8]; };

static void sys_connect(struct syscall_registers *regs) {
    int fd = (int)regs->ebx;
    struct _sys_sockaddr_in *addr = (struct _sys_sockaddr_in *)regs->ecx;

    if (fd < 0 || fd >= MAX_FDS) { regs->eax = -1; return; }
    int sys_fd = current_task->fd_table[fd];
    if (sys_fd == -1) { regs->eax = -1; return; }

    ip_addr_t ip; ip.addr = addr->s_addr; 
    KFile *kf = &system_open_files[sys_fd];

    if (kf->type == FILE_TYPE_SOCKET) {
        raw_connect((struct raw_pcb *)kf->socket_id, &ip);
        regs->eax = 0;
    } else if (kf->type == FILE_TYPE_SOCKET_TCP) {
        if (kf->offset == 0) { 
            kf->offset = 1; 
            kf->is_dirty = 0;
            tcp_connect((struct tcp_pcb *)kf->socket_id, &ip, ntohs(addr->sin_port), tcp_connected_callback);
            regs->eax = 0xFFFFFFFF; 
            return;
        }

        if (kf->is_dirty == 1) {
            kf->offset = 0; kf->is_dirty = 0; regs->eax = 0; 
        } else if (kf->is_dirty == -1) {
            kf->offset = 0; kf->is_dirty = 0; regs->eax = -1; 
        } else {
            regs->eax = 0xFFFFFFFF; 
        }
    } else { regs->eax = -1; }
}

static void sys_send(struct syscall_registers *regs) {
    int fd = (int)regs->ebx; void *data = (void *)regs->ecx; size_t size = (size_t)regs->edx;
    if (fd < 0 || fd >= MAX_FDS) { regs->eax = -1; return; }
    int sys_fd = current_task->fd_table[fd];
    if (sys_fd == -1) { regs->eax = -1; return; }

    KFile *kf = &system_open_files[sys_fd];

    if (kf->type == FILE_TYPE_SOCKET) {
        struct pbuf *p = pbuf_alloc(PBUF_IP, size, PBUF_RAM);
        pbuf_take(p, data, size);
        regs->eax = raw_send((struct raw_pcb *)kf->socket_id, p);
        pbuf_free(p);
    } else if (kf->type == FILE_TYPE_SOCKET_TCP) {
        struct tcp_pcb *pcb = (struct tcp_pcb *)kf->socket_id;
        err_t err = tcp_write(pcb, data, size, TCP_WRITE_FLAG_COPY);
        tcp_output(pcb);
        regs->eax = (err == ERR_OK) ? size : -1;
    } else { regs->eax = -1; }
}

static void sys_recv(struct syscall_registers *regs) {
    int fd = (int)regs->ebx; void *mem = (void *)regs->ecx; size_t len = (size_t)regs->edx;

    if (fd < 0 || fd >= MAX_FDS) { regs->eax = -1; return; }
    int sys_fd = current_task->fd_table[fd];
    if (sys_fd == -1) { regs->eax = -1; return; }

    KFile *kf = &system_open_files[sys_fd];
    
    if (kf->is_dirty == 0) { regs->eax = 0xFFFFFFFF; return; } 
    if (kf->is_dirty == -1 && kf->size == 0) { regs->eax = 0; return; } 

    uint32_t to_copy = kf->size > len ? len : kf->size;
    fast_memcpy(mem, kf->buffer, to_copy);
    
    if (kf->size > to_copy) {
        memmove(kf->buffer, kf->buffer + to_copy, kf->size - to_copy);
        kf->size -= to_copy;
    } else {
        kf->size = 0;
        kf->is_dirty = (kf->is_dirty == -1) ? -1 : 0;
    }

    if (kf->type == FILE_TYPE_SOCKET_TCP && to_copy > 0) {
        tcp_recved((struct tcp_pcb *)kf->socket_id, to_copy);
    }

    regs->eax = to_copy;
}



static volatile int dns_state = 0; 
static ip_addr_t resolved_ip;
static char kernel_hostname[256]; 

static void dns_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    if (ipaddr != NULL) {
        resolved_ip = *ipaddr;
        dns_state = 2;
    } else {
        dns_state = 3;
    }
}

static void sys_gethostbyname(struct syscall_registers *regs) {
    char* user_hostname = (char*)regs->ebx;

    if (dns_state == 2) {
        dns_state = 0;
        regs->eax = resolved_ip.addr;
        return;
    } else if (dns_state == 3) {
        dns_state = 0;
        regs->eax = 0;
        return;
    }

    if (dns_state == 0) {
        
        int i = 0;
        while(user_hostname[i] && i < 255) {
            kernel_hostname[i] = user_hostname[i];
            i++;
        }
        kernel_hostname[i] = '\0';

        ip_addr_t addr;
        
        err_t err = dns_gethostbyname(kernel_hostname, &addr, dns_callback, NULL);
        if (err == ERR_OK) {
            regs->eax = addr.addr; 
            return;
        } else if (err == ERR_INPROGRESS) {
            dns_state = 1;
            regs->eax = 0xFFFFFFFF; 
            return;
        } else {
            regs->eax = 0; 
            return;
        }
    }
    
    regs->eax = 0xFFFFFFFF; 
}

static void sys_scancode_to_char(struct syscall_registers *regs) {
    regs->eax = (char)scancode_to_char((uint8_t)regs->ebx);
}

static void sys_wait_scancode(struct syscall_registers *regs) {
    regs->eax = wait_scancode();
}

static void sys_get_rtc(struct syscall_registers *regs) {
    struct time *t_ptr = (struct time *)regs->ebx;
    if (t_ptr) {
        *t_ptr = rtc_get_time();
    }
}

static void sys_sigaction(struct syscall_registers *regs) {
    int sig = regs->ebx;
    uint32_t handler = regs->ecx;
    if (sig > 0 && sig < 32 && current_task) {
        current_task->signal_handlers[sig] = handler;
        regs->eax = 0;
    } else regs->eax = -1;
}

static void sys_sigreturn(struct syscall_registers *regs) {
    extern void sys_sigreturn_impl(uint32_t esp);
    sys_sigreturn_impl((uint32_t)regs); 
    
}

static syscall_handler_t syscall_table[256] = {
    [1]  = sys_exit, [2]  = sys_fork, [3]  = sys_read, [4]  = sys_write, [5]  = sys_read_file, [6]  = sys_write_file, 
    [7]  = sys_waitpid, [8]  = sys_get_file_size, [10] = sys_delete_file, [11] = sys_spawn, [12] = sys_chdir, [13] = sys_set_color,     
    [14] = sys_get_tasks, [15] = sys_get_mem_info, [16] = sys_get_cursor, [17] = sys_set_cursor, [18] = sys_clear, [19] = sys_getenv,
    [20] = sys_getuid, [21] = sys_setuid, [22] = sys_get_term_size, [23] = sys_print,
    [37] = sys_kill, [39] = sys_mkdir, [45] = sys_malloc, [46] = sys_free, [79] = sys_getcwd, [89] = sys_readdir,   
    [90] = sys_dlopen, [91] = sys_dlsym,
    [92] = sys_mount, [93] = sys_unmount,
    [24] = sys_detach, [25] = sys_get_screen_info, [26] = sys_flush_screen, 
    [27] = sys_get_mouse, [28] = sys_poll_key, [29] = sys_get_key_modifiers, [30] = sys_yield,
    [31] = sys_get_ticks, [32] = sys_get_key_state,
    
    [100] = wrap_open, [101] = wrap_close, [102] = wrap_lseek, [103] = sys_sbrk,
    [104] = wrap_read_fd, [105] = wrap_write_fd,
    [110] = sys_shm_get, [111] = sys_shm_map,
    [112] = sys_pipe, [113] = sys_spawn_ext,
    
    [120] = sys_socket, [121] = sys_connect, [122] = sys_send, [123] = sys_recv, 
    [124] = sys_gethostbyname,
    [125] = sys_scancode_to_char,
    [126] = sys_wait_scancode,
    [33] = sys_get_rtc,
    [118] = sys_sigaction, [119] = sys_sigreturn,
};

extern void check_signals(uint32_t esp);

void syscall_handler_c(struct syscall_registers *regs) {
    if (regs->eax < 256 && syscall_table[regs->eax] != NULL) 
        syscall_table[regs->eax](regs); 
    else 
        printf("Kernel Warning: Unhandled syscall ID %d\n", regs->eax);
    
    check_signals((uint32_t)regs); 
}