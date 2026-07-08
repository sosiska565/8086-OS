/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/kernel/task/task.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */



#include "task/task.h"
#include "mm/memory.h"
#include "drivers/vga/vga.h"
#include "drivers/io/io.h"
#include "drivers/timer/timer.h"
#include "utils/utils.h"
#include "fs/vfs.h"
#include "include/elf.h"
#include "fs/fd.h"

#define PROC_STACK_SIZE 1048576

extern void switch_to_task(Task *next, Task *prev);

Task *current_task = 0;
Task *ready_queue = 0;
int next_pid = 0;
Task *zombie_task = 0; 
volatile int tasks_need_cleanup = 0; 

int foreground_task_id = 0; 

void track_allocation(Task *task, void *ptr) {
    if (!task || !ptr) return;
    AllocList *node = (AllocList*)kmalloc(sizeof(AllocList));
    if (!node) return; 
    node->ptr = ptr; node->next = task->allocations; task->allocations = node;
}

void track_allocation_a(Task *task, void *ptr) {
    if (!task || !ptr) return;
    AllocList *node = (AllocList*)kmalloc(sizeof(AllocList));
    if (!node) return;
    node->ptr = (void*)((uint32_t)ptr | 1); node->next = task->allocations; task->allocations = node;
}

void untrack_allocation(Task *task, void *ptr) {
    if (!task || !task->allocations) return;
    AllocList *curr = task->allocations; AllocList *prev = 0;
    while (curr) {
        uint32_t val = (uint32_t)curr->ptr;
        if ((void*)(val & ~1) == ptr) { 
            if (prev) prev->next = curr->next; else task->allocations = curr->next;
            kfree(curr); return;
        }
        prev = curr; curr = curr->next;
    }
}

void free_task_memory(Task *task) {
    AllocList *curr = task->allocations;
    while (curr) { 
        AllocList *temp = curr; uint32_t val = (uint32_t)curr->ptr;
        if (val & 1) kfree_a((void*)(val & ~1)); else kfree((void*)val);
        curr = curr->next; kfree(temp); 
    }
    task->allocations = 0;
}

void free_task_struct(Task *t) {
    if (!t) return;
    free_task_memory(t); 
    
    if (t->app_phys_addr) kfree_a((void*)t->app_phys_addr);
    
    if (t->page_dir && t->page_dir != kernel_dir) {
        for (int i = 256; i < 1024; i++) {
            if ((t->page_dir->entries[i] & 1) && (t->page_dir->entries[i] != kernel_dir->entries[i])) {
                
                uint32_t aligned_addr = t->page_dir->entries[i] & 0xFFFFF000;
                
                kfree_a((void*)aligned_addr);
            }
        }
        kfree_a(t->page_dir);
    }
    
    if (t->stack_start) kfree((void*)t->stack_start);
    kfree(t);
}

void cleanup_zombies(void) {
    if(!tasks_need_cleanup) return;
    uint32_t flags = save_flags();
    if(ready_queue) {
        Task *curr = ready_queue;
        Task *start = ready_queue;
        int looped = 0;
        
        while(curr && !looped) {
            Task *next_task = curr->next;
            if(next_task == start) looped = 1;
            
            if(curr->state == TASK_DEAD) {
                Task *to_free = curr;
                if (curr->next == curr) {
                    ready_queue = NULL;
                    free_task_struct(to_free);
                    break;
                } else {
                    Task *prev = ready_queue;
                    while(prev->next != curr) prev = prev->next;
                    prev->next = curr->next;
                    
                    if(curr == ready_queue) ready_queue = curr->next;
                    if(curr == start) start = curr->next;
                    
                    
                    if (current_task == curr) current_task = curr->next; 
                    
                    free_task_struct(to_free);
                }
            }
            curr = next_task;
        }
    }
    tasks_need_cleanup = 0;
    zombie_task = 0;
    restore_flags(flags);
}

void task_set_name(Task* t, char* name) {
    int i = 0;
    if (!name) { strcpy(t->name, "unk"); return; }
    while (name[i] && i < 31) { t->name[i] = name[i]; i++; } t->name[i] = 0;
}

void init_tasking() {
    Task *ktask = (Task*)kmalloc(sizeof(Task));
    ktask->id = next_pid++; ktask->esp = 0; ktask->stack_start = 0; ktask->next = ktask; 
    ktask->state = TASK_RUNNING; ktask->parent_id = -1; ktask->allocations = 0;
    ktask->page_dir = kernel_dir; ktask->app_phys_addr = 0; ktask->kill_me = 0; ktask->wake_tick = 0;
    ktask->cwd[0] = '/'; ktask->cwd[1] = '\0';
    ktask->uid = 0;
    ktask->heap_start = 0x60000000; ktask->heap_end = 0x60000000;
    ktask->pending_signals = 0;
    ktask->in_signal_handler = 0;
    for(int i=0; i<MAX_SIG; i++) ktask->signal_handlers[i] = 0;
    
    for(int i = 0; i < MAX_FDS; i++) ktask->fd_table[i] = -1; 
    
    task_set_name(ktask, "kernel");
    current_task = ktask; ready_queue = ktask;
    foreground_task_id = ktask->id;
    __asm__ volatile ("sti");
}

int create_process(void (*entry)(int, char**), int argc, char **argv, char *name, struct page_directory_t *pd, int fd_in, int fd_out) {
    cleanup_zombies();
    Task *new_task = (Task*)kmalloc(sizeof(Task));
    if (!new_task) return -1;

    new_task->id = next_pid++; new_task->state = TASK_READY; new_task->kill_me = 0;
    new_task->allocations = 0; new_task->page_dir = pd ? pd : kernel_dir;
    new_task->app_phys_addr = 0; new_task->wake_tick = 0;
    new_task->heap_start = 0x60000000; new_task->heap_end = 0x60000000;
    new_task->pending_signals = 0;
    new_task->in_signal_handler = 0;
    if (current_task) {
        for(int i=0; i<MAX_SIG; i++) new_task->signal_handlers[i] = current_task->signal_handlers[i];
    } else {
        for(int i=0; i<MAX_SIG; i++) new_task->signal_handlers[i] = 0;
    }

    task_set_name(new_task, name);
    if (current_task) {
        new_task->parent_id = current_task->id;
        new_task->uid = current_task->uid; 
        strcpy(new_task->cwd, current_task->cwd);
        
        fd_inherit(new_task, current_task);
        
        if (fd_in != -1) {
            int old_sys = new_task->fd_table[0];
            if (old_sys != -1) {
                system_open_files[old_sys].refcount--;
                if (system_open_files[old_sys].type == FILE_TYPE_PIPE) {
                    if (system_open_files[old_sys].mode == O_RDONLY) system_open_files[old_sys].pipe->readers--;
                    else system_open_files[old_sys].pipe->writers--;
                }
            }
            int new_sys = current_task->fd_table[fd_in];
            new_task->fd_table[0] = new_sys;
            if (new_sys != -1) {
                system_open_files[new_sys].refcount++;
                if (system_open_files[new_sys].type == FILE_TYPE_PIPE) {
                    if (system_open_files[new_sys].mode == O_RDONLY) system_open_files[new_sys].pipe->readers++;
                    else system_open_files[new_sys].pipe->writers++;
                }
            }
        }
        
        if (fd_out != -1) {
            int old_sys = new_task->fd_table[1];
            if (old_sys != -1) {
                system_open_files[old_sys].refcount--;
                if (system_open_files[old_sys].type == FILE_TYPE_PIPE) {
                    if (system_open_files[old_sys].mode == O_RDONLY) system_open_files[old_sys].pipe->readers--;
                    else system_open_files[old_sys].pipe->writers--;
                }
            }
            int new_sys = current_task->fd_table[fd_out];
            new_task->fd_table[1] = new_sys;
            if (new_sys != -1) {
                system_open_files[new_sys].refcount++;
                if (system_open_files[new_sys].type == FILE_TYPE_PIPE) {
                    if (system_open_files[new_sys].mode == O_RDONLY) system_open_files[new_sys].pipe->readers++;
                    else system_open_files[new_sys].pipe->writers++;
                }
            }
        }
    } else {
        new_task->parent_id = -1; new_task->uid = 0;
        new_task->cwd[0] = '/'; new_task->cwd[1] = '\0';
        for(int i = 0; i < MAX_FDS; i++) new_task->fd_table[i] = -1;
    }

    uint32_t *stack = (uint32_t*)kmalloc(PROC_STACK_SIZE);
    if (!stack) { kfree(new_task); return -1; }
    new_task->stack_start = (uint32_t)stack;
    char *stack_top = (char*)stack + PROC_STACK_SIZE;

    char **new_argv = 0;
    if (argc > 0 && argv) {
        uint32_t ptrs[64]; 
        for (int i = argc - 1; i >= 0; i--) {
            int len = strlen(argv[i]) + 1; stack_top -= len;
            strcpy(stack_top, argv[i]); ptrs[i] = (uint32_t)stack_top;
        }
        stack_top = (char*)((uint32_t)stack_top & ~3); stack_top -= (argc + 1) * sizeof(char*);
        new_argv = (char**)stack_top;
        for (int i = 0; i < argc; i++) new_argv[i] = (char*)ptrs[i];
        new_argv[argc] = 0;
    }

    uint32_t *esp = (uint32_t*)stack_top;
    extern void exit_process(); 
    *(--esp) = (uint32_t)new_argv; *(--esp) = (uint32_t)argc;
    *(--esp) = (uint32_t)exit_process; *(--esp) = (uint32_t)entry;
    for(int i = 0; i < 8; i++) *(--esp) = 0;
    *(--esp) = 0x202;
    new_task->esp = (uint32_t)esp;

    uint32_t flags = save_flags();
    Task *temp = ready_queue->next; ready_queue->next = new_task; new_task->next = temp;
    restore_flags(flags); 

    char log[128]; 
    sprintf(log, "[TASK] Process spawned: '%s' (PID %d)", name, new_task->id); 
    klog(log);

    return new_task->id;
}

void task_scheduler() {
    uint32_t flags = save_flags();
    cleanup_zombies();
    if (!current_task || !ready_queue) return;
    Task *prev = current_task; Task *next = prev->next;
    while (1) {
        if (next->state == TASK_RUNNING || next->state == TASK_READY) break;
        if (next->state == TASK_SLEEPING && get_ticks() >= next->wake_tick) { next->state = TASK_RUNNING; break; }
        next = next->next;
        if (next == prev) {
            if (prev->state == TASK_RUNNING) break;
            
            restore_flags(flags); 
            return; 
        }
    }
    if (next == prev) { restore_flags(flags); return; }
    current_task = next;
    if (next->page_dir && next->page_dir != current_dir) switch_page_directory(next->page_dir);
    switch_to_task(next, prev);
    restore_flags(flags);
}

void kill_children_of(int pid) {
    if (!ready_queue) return;
    Task *t = ready_queue;
    do { if (t->parent_id == pid && t->state != TASK_DEAD) t->parent_id = 1; t = t->next; } while (t != ready_queue);
}

void exit_process() {
    __asm__ volatile("cli");
    char log[128];
    sprintf(log, "[TASK] Process exited/killed: '%s' (PID %d)", current_task->name, current_task->id);
    klog(log);

    kill_children_of(current_task->id);

    extern struct {
        int key;
        uint32_t phys_addr;
        uint32_t size;
        int owner_pid;
    } shm_table[32];
    extern void kfree_a(void* ptr);

    for (int i = 0; i < 32; i++) {
        if (shm_table[i].phys_addr != 0 && shm_table[i].owner_pid == current_task->id) {
            char shm_log[128];
            sprintf(shm_log, "[SHM] Recovered leaked memory block (Key: %d, Size: %d)", shm_table[i].key, shm_table[i].size);
            klog(shm_log);
            kfree_a((void*)shm_table[i].phys_addr);
            shm_table[i].phys_addr = 0;
            shm_table[i].key = 0;
            shm_table[i].size = 0;
            shm_table[i].owner_pid = 0;
        }
    }

    for (int i = 0; i < MAX_FDS; i++) {
        if (current_task->fd_table[i] != -1)
            sys_close(i);
    }

    Task *task_to_kill = current_task;
    task_to_kill->state = TASK_DEAD;
    tasks_need_cleanup = 1;

    Task *next_live = task_to_kill->next;
    while (next_live->state == TASK_DEAD && next_live != task_to_kill) {
        next_live = next_live->next;
    }
    current_task = next_live;

    if (next_live->state == TASK_SLEEPING) {
        next_live->state = TASK_READY;
    }

    if (foreground_task_id == task_to_kill->id)
        foreground_task_id = current_task->id;

    if (next_live->page_dir)
        switch_page_directory(next_live->page_dir);
    else
        switch_page_directory(kernel_dir);

    switch_to_task(next_live, task_to_kill);
    while (1) { __asm__ volatile("hlt"); }
}

void yield() { task_scheduler(); }
void create_thread(void (*f)(void)) { create_process((void (*)(int, char**))f, 0, 0, "thread", kernel_dir, -1, -1); }
void task_sleep(int ms) { 
    if(!current_task) return; current_task->wake_tick = get_ticks() + ms;
    current_task->state = TASK_SLEEPING; task_scheduler(); 
}
void check_kill_flag() { if (current_task && current_task->kill_me) { current_task->kill_me = 0; exit_process(); } }

void wait_process(int pid) {
    foreground_task_id = pid; 
    while (1) {
        Task *t = ready_queue; int found = 0;
        do { if (t->id == pid && t->state != TASK_DEAD) { found = 1; break; } t = t->next; } while (t != ready_queue);
        if (!found) break; task_sleep(20);
    }
    foreground_task_id = current_task->id; 
}

void kill_task(int pid) {
    if (!ready_queue) return;
    if (pid <= 1) return;
    Task *t = ready_queue; int found = 0;
    do { if (t->id == pid && t->state != TASK_DEAD) { found = 1; break; } t = t->next; } while (t != ready_queue);
    if (!found) return;
    if (t == current_task) { exit_process(); return; }
    t->kill_me = 1;
}

int spawn_process_ext(char* path, char** argv, int fd_in, int fd_out) {
    int file_size = vfs_get_size(path);
    if (file_size <= 0) return -1;

    uint8_t *file_buf = (uint8_t*)kmalloc(file_size);
    if (!file_buf) return -1;
    vfs_read(path, file_buf);

    page_directory_t *app_pd = clone_page_directory();
    uint32_t entry_point = 0x60000000;
    uint32_t next_free_page = 0x60000000; 
    uint32_t allocs[32]; int alloc_cnt = 0;
    Elf32_Ehdr *hdr = (Elf32_Ehdr*)file_buf;

    if (*(uint32_t*)hdr->e_ident == ELF_MAGIC) {
        entry_point = hdr->e_entry;
        Elf32_Phdr *phdrs = (Elf32_Phdr*)(file_buf + hdr->e_phoff);
        uint32_t min_vaddr = 0xFFFFFFFF; uint32_t max_vaddr = 0;
        
        for (int i = 0; i < hdr->e_phnum; i++) {
            if (phdrs[i].p_type == PT_LOAD) { 
                if (phdrs[i].p_vaddr < min_vaddr) min_vaddr = phdrs[i].p_vaddr;
                uint32_t end_vaddr = phdrs[i].p_vaddr + phdrs[i].p_memsz;
                if (end_vaddr > max_vaddr) max_vaddr = end_vaddr;
            }
        }

        if (min_vaddr != 0xFFFFFFFF) {
            uint32_t base_page = min_vaddr & 0xFFFFF000;
            uint32_t top_page = (max_vaddr + 4095) & ~4095;
            uint32_t total_alloc_size = top_page - base_page;
            
            next_free_page = top_page; 
            
            uint32_t phys_addr = (uint32_t)kmalloc_a(total_alloc_size);
            if (!phys_addr) { for(int c = 0; c < alloc_cnt; c++) kfree_a((void*)allocs[c]); kfree(file_buf); return -1; }
            if (alloc_cnt < 32) allocs[alloc_cnt++] = phys_addr;
            
            fast_memset((void*)phys_addr, 0, total_alloc_size / 4);
            for (int i = 0; i < hdr->e_phnum; i++) {
                if (phdrs[i].p_type == PT_LOAD) {
                    uint32_t dest_offset = phdrs[i].p_vaddr - base_page;
                    fast_memcpy((void*)(phys_addr + dest_offset), file_buf + phdrs[i].p_offset, phdrs[i].p_filesz);
                }
            }
            for (uint32_t j = 0; j < total_alloc_size; j += 4096) {
                paging_map_user(app_pd, phys_addr + j, base_page + j, 7);
            }
        }
    } else {
        uint32_t alloc_size = (file_size + 1024 * 1024 + 4095) & ~4095; 
        uint32_t phys_addr = (uint32_t)kmalloc_a(alloc_size);
        if (!phys_addr) { kfree(file_buf); return -1; }
        if (alloc_cnt < 32) allocs[alloc_cnt++] = phys_addr;
        fast_memset((void*)phys_addr, 0, alloc_size / 4);
        fast_memcpy((void*)phys_addr, file_buf, file_size);
        for(uint32_t i = 0; i < alloc_size; i += 4096) paging_map_user(app_pd, phys_addr + i, 0x60000000 + i, 7);
        
        next_free_page = 0x60000000 + alloc_size; 
    }
    kfree(file_buf);

    int argc = 0; if (argv) { while(argv[argc]) argc++; }
    char* name = path; for(int i = 0; path[i]; i++) if(path[i] == '/') name = &path[i+1];

    uint32_t flags = save_flags(); 
    int pid = create_process((void (*)(int, char**))entry_point, argc, argv, name, app_pd, fd_in, fd_out);
    
    if (pid < 0) {
        for(int c=0; c<alloc_cnt; c++) kfree_a((void*)allocs[c]);
        
        if (app_pd && app_pd != kernel_dir) {
            for (int i = 256; i < 1024; i++) {
                if (app_pd->entries[i] & 1 && app_pd->entries[i] != kernel_dir->entries[i]) {
                    kfree_a((void*)(app_pd->entries[i] & 0xFFFFF000));
                }
            }
        }
        
        kfree_a(app_pd); 
        restore_flags(flags); 
        return -1;
    }

    Task *t = ready_queue;
    do {
        if (t->id == pid) { 
            t->lib_offset = 0x70000000; 
            t->heap_start = next_free_page; 
            t->heap_end = next_free_page;   
            for(int i=0; i<alloc_cnt; i++) track_allocation_a(t, (void*)allocs[i]);
            break; 
        }
        t = t->next;
    } while (t != ready_queue);

    restore_flags(flags); 
    return pid;
}

int spawn_process(char* path, char** argv) {
    return spawn_process_ext(path, argv, -1, -1);
}

uint32_t load_library(Task* t, char* lib_name) {
    char path[128];
    if (lib_name[0] == '/') strcpy(path, lib_name);
    else { strcpy(path, "/lib/"); strcat(path, lib_name); } 

    int size = vfs_get_size(path);
    if (size <= 0) return 0;

    uint8_t *file_buf = kmalloc(size);
    vfs_read(path, file_buf);

    Elf32_Ehdr *hdr = (Elf32_Ehdr*)file_buf;
    if (*(uint32_t*)hdr->e_ident != ELF_MAGIC) { kfree(file_buf); return 0; }

    uint32_t base_addr = t->lib_offset;
    Elf32_Phdr *phdrs = (Elf32_Phdr*)(file_buf + hdr->e_phoff);

    for (int i = 0; i < hdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            uint32_t mem_sz = phdrs[i].p_memsz;
            uint32_t alloc_size = (mem_sz + 4095) & ~4095;
            uint32_t phys_addr = (uint32_t)kmalloc_a(alloc_size);
            if (!phys_addr) continue;

            track_allocation_a(t, (void*)phys_addr); 
            
            fast_memset((void*)phys_addr, 0, alloc_size / 4);
            fast_memcpy((void*)phys_addr, file_buf + phdrs[i].p_offset, phdrs[i].p_filesz);

            for (uint32_t j = 0; j < alloc_size; j += 4096) {
                paging_map_user(t->page_dir, phys_addr + j, base_addr + (phdrs[i].p_vaddr & 0xFFFFF000) + j, 7);
            }
        }
    }

    track_allocation(t, file_buf); 
    t->lib_offset += 0x01000000; 
    return (uint32_t)file_buf; 
}


uint32_t get_symbol(Task* t, uint32_t lib_handle, char* sym_name) {
    if (!lib_handle) return 0;
    uint8_t *file_buf = (uint8_t*)lib_handle;
    Elf32_Ehdr *hdr = (Elf32_Ehdr*)file_buf;
    Elf32_Shdr *shdrs = (Elf32_Shdr*)(file_buf + hdr->e_shoff);

    Elf32_Shdr *symtab = NULL;
    Elf32_Shdr *strtab = NULL;

    for (int i = 0; i < hdr->e_shnum; i++) {
        if (shdrs[i].sh_type == SHT_SYMTAB) symtab = &shdrs[i];
        if (shdrs[i].sh_type == SHT_STRTAB && i != hdr->e_shstrndx) strtab = &shdrs[i];
    }

    if (!symtab || !strtab) return 0;

    Elf32_Sym *syms = (Elf32_Sym*)(file_buf + symtab->sh_offset);
    int num_syms = symtab->sh_size / symtab->sh_entsize;
    char *strings = (char*)(file_buf + strtab->sh_offset);

    for (int i = 0; i < num_syms; i++) {
        char *name = strings + syms[i].st_name;
        if (strcmp(name, sym_name) == 0) {
            uint32_t base = 0x50000000; 
            return base + syms[i].st_value;
        }
    }
    return 0;
}

void send_signal(int pid, int sig) {
    if (!ready_queue || sig < 1 || sig >= MAX_SIG) return;
    if (pid <= 1) {
        return; 
    }

    Task *t = ready_queue;
    do {
        if (t->id == pid && t->state != TASK_DEAD) {
            if (sig == SIGKILL) {
                t->kill_me = 1;
            } else {
                t->pending_signals |= (1 << sig);
            }
            return;
        }
        t = t->next;
    } while (t != ready_queue);
}


extern void signal_trampoline_entry(void); 

void check_signals(uint32_t esp_after_pusha) {
    if (!current_task || current_task->id <= 1 || current_task->in_signal_handler) return;
    if (current_task->pending_signals == 0) return;

    struct trap_frame *frame = (struct trap_frame *)esp_after_pusha;

    for (int sig = 1; sig < MAX_SIG; sig++) {
        if (current_task->pending_signals & (1 << sig)) {
            current_task->pending_signals &= ~(1 << sig); 

            uint32_t handler = current_task->signal_handlers[sig];
            
            
            if (handler == 1) { 
                continue; 
            }

            
            if (handler == 0) { 
                if (sig == SIGINT || sig == SIGKILL) {
                    current_task->kill_me = 1; 
                }
                return;
            }

            
            fast_memcpy(&current_task->sig_saved_frame, frame, sizeof(struct trap_frame));
            current_task->in_signal_handler = 1;

            frame->eip = (uint32_t)signal_trampoline_entry; 
            frame->eax = handler;                           
            frame->ebx = sig;                               
            
            return;
        }
    }
}

void sys_sigreturn_impl(uint32_t esp_after_pusha) {
    if (!current_task || !current_task->in_signal_handler) return;
    struct trap_frame *frame = (struct trap_frame *)esp_after_pusha;
    
    
    fast_memcpy(frame, &current_task->sig_saved_frame, sizeof(struct trap_frame));
    current_task->in_signal_handler = 0;
}
