#include "task/task.h"
#include "mm/memory.h"
#include "drivers/vga/vga.h"
#include "drivers/io/io.h"
#include "drivers/timer/timer.h"
#include "utils/utils.h"
#include "fs/vfs.h"
#include "include/elf.h"

extern void switch_to_task(Task *next, Task *prev);

Task *current_task = 0;
Task *ready_queue = 0;
int next_pid = 0;
Task *zombie_task = 0; 
volatile int tasks_need_cleanup = 0; 

int foreground_task_id = 0; 


char* current_spawn_redirect = NULL;

void track_allocation(Task *task, void *ptr) {
    if (!task || !ptr) return;
    AllocList *node = (AllocList*)kmalloc(sizeof(AllocList));
    if (!node) return; node->ptr = ptr; node->next = task->allocations; task->allocations = node;
}

void untrack_allocation(Task *task, void *ptr) {
    if (!task || !task->allocations) return;
    AllocList *curr = task->allocations; AllocList *prev = 0;
    while (curr) {
        if (curr->ptr == ptr) {
            if (prev) prev->next = curr->next; else task->allocations = curr->next;
            kfree(curr); return;
        }
        prev = curr; curr = curr->next;
    }
}

void free_task_memory(Task *task) {
    AllocList *curr = task->allocations;
    while (curr) { AllocList *temp = curr; kfree(curr->ptr); curr = curr->next; kfree(temp); }
    task->allocations = 0;
}

void free_task_struct(Task *t) {
    if (!t) return;
    free_task_memory(t);
    if (t->app_phys_addr) {
        if (t->page_dir && t->page_dir != kernel_dir) {
            for (int i = 256; i < 1024; i++) {
                if (t->page_dir->entries[i] & 1 && t->page_dir->entries[i] != kernel_dir->entries[i]) {
                    kfree_a((void*)(t->page_dir->entries[i] & 0xFFFFF000));
                }
            }
            kfree_a(t->page_dir);
        }
        kfree_a((void*)t->app_phys_addr);
    }
    if (t->stack_start) kfree((void*)t->stack_start);
    kfree(t);
}

void cleanup_zombies() {
    if (!tasks_need_cleanup && !zombie_task) return;
    uint32_t flags = save_flags();
    if (zombie_task != 0) { free_task_struct(zombie_task); zombie_task = 0; }
    if (ready_queue) {
        Task *curr = ready_queue; Task *prev = ready_queue;
        while (prev->next != ready_queue) prev = prev->next;
        Task *start = ready_queue;
        do {
            if (curr->state == TASK_DEAD) {
                if (curr == ready_queue) { ready_queue = curr->next; start = ready_queue; }
                prev->next = curr->next;
                Task *to_free = curr; curr = curr->next;
                if (ready_queue == to_free) ready_queue = 0; 
                free_task_struct(to_free);
                if (!ready_queue) break;
            } else { prev = curr; curr = curr->next; }
        } while (curr != start && ready_queue != 0);
    }
    tasks_need_cleanup = 0; restore_flags(flags);
}

void task_set_name(Task* t, char* name) {
    int i = 0;
    if (!name) { t->name[0] = 'u'; t->name[1] = 'n'; t->name[2] = 'k'; t->name[3] = 0; return; }
    while (name[i] && i < 31) { t->name[i] = name[i]; i++; } t->name[i] = 0;
}

void init_tasking() {
    Task *ktask = (Task*)kmalloc(sizeof(Task));
    ktask->id = next_pid++; ktask->esp = 0; ktask->stack_start = 0; ktask->next = ktask; 
    ktask->state = TASK_RUNNING; ktask->parent_id = -1; ktask->allocations = 0;
    ktask->page_dir = kernel_dir; ktask->app_phys_addr = 0; ktask->kill_me = 0; ktask->wake_tick = 0;
    ktask->cwd[0] = '/'; ktask->cwd[1] = '\0';
    ktask->uid = 0; 
    ktask->redirect_buf = 0;
    
    for(int i = 0; i < MAX_FDS; i++) ktask->fd_table[i] = (i < 3) ? i : -1;
    task_set_name(ktask, "kernel");
    current_task = ktask; ready_queue = ktask;
    foreground_task_id = ktask->id;
    __asm__ volatile ("sti");
}

int create_process(void (*entry)(int, char**), int argc, char **argv, char *name, struct page_directory_t *pd) {
    cleanup_zombies();
    Task *new_task = (Task*)kmalloc(sizeof(Task));
    new_task->id = next_pid++; new_task->state = TASK_READY; new_task->kill_me = 0;
    new_task->allocations = 0; new_task->page_dir = pd ? pd : kernel_dir;
    new_task->app_phys_addr = 0; new_task->wake_tick = 0;

    
    if (current_spawn_redirect) {
        strcpy(new_task->redirect_path, current_spawn_redirect);
        new_task->redirect_buf = kmalloc(65536);
        new_task->redirect_size = 0;
        current_spawn_redirect = NULL; 
    } else {
        new_task->redirect_buf = 0;
    }

    task_set_name(new_task, name);
    if (current_task) {
        new_task->parent_id = current_task->id;
        new_task->uid = current_task->uid; 
        strcpy(new_task->cwd, current_task->cwd);
        for(int i = 0; i < MAX_FDS; i++) new_task->fd_table[i] = current_task->fd_table[i];
    } else {
        new_task->parent_id = -1; new_task->uid = 0;
        new_task->cwd[0] = '/'; new_task->cwd[1] = '\0';
    }

    uint32_t *stack = (uint32_t*)kmalloc(65536);
    new_task->stack_start = (uint32_t)stack;
    
    char *stack_top = (char*)stack + 65536;

    char **new_argv = 0;
    if (argc > 0 && argv) {
        uint32_t ptrs[64]; 
        for (int i = argc - 1; i >= 0; i--) {
            int len = strlen(argv[i]) + 1;
            stack_top -= len;
            strcpy(stack_top, argv[i]);
            ptrs[i] = (uint32_t)stack_top;
        }
        stack_top = (char*)((uint32_t)stack_top & ~3); 
        stack_top -= (argc + 1) * sizeof(char*);
        new_argv = (char**)stack_top;
        for (int i = 0; i < argc; i++) new_argv[i] = (char*)ptrs[i];
        new_argv[argc] = 0;
    }

    uint32_t *esp = (uint32_t*)stack_top;
    extern void exit_process(); 

    *(--esp) = (uint32_t)new_argv; 
    *(--esp) = (uint32_t)argc;
    *(--esp) = (uint32_t)exit_process; 
    *(--esp) = (uint32_t)entry;
    for(int i = 0; i < 8; i++) *(--esp) = 0;
    *(--esp) = 0x202;
    new_task->esp = (uint32_t)esp;

    __asm__ volatile("cli");
    Task *temp = ready_queue->next; ready_queue->next = new_task; new_task->next = temp;
    __asm__ volatile("sti");
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
        if (next == prev) { if (prev->state == TASK_RUNNING) break; return; }
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
    do {
        if (t->parent_id == pid && t->state != TASK_DEAD) {
            kill_children_of(t->id); 
            if (t == current_task) t->kill_me = 1;
            else { t->state = TASK_DEAD; tasks_need_cleanup = 1; }
        }
        t = t->next;
    } while (t != ready_queue);
}

void exit_process() {
    __asm__ volatile("cli");
    outb(0x20, 0x20);

    kill_children_of(current_task->id);

    if (current_task->redirect_buf != 0) {
        vfs_write(current_task->redirect_path, current_task->redirect_buf, current_task->redirect_size);
        kfree(current_task->redirect_buf);
        current_task->redirect_buf = 0;
    }

    Task *task_to_kill = current_task;
    Task *prev = ready_queue;
    while (prev->next != task_to_kill) { prev = prev->next; }
    prev->next = task_to_kill->next;
    if (ready_queue == task_to_kill) ready_queue = task_to_kill->next;
    
    task_to_kill->state = TASK_DEAD;
    zombie_task = task_to_kill; tasks_need_cleanup = 1;

    Task *next_live = task_to_kill->next;
    current_task = next_live;
    if (foreground_task_id == task_to_kill->id) foreground_task_id = current_task->id; 

    if (next_live->page_dir) switch_page_directory(next_live->page_dir);
    else switch_page_directory(kernel_dir);
    
    switch_to_task(next_live, task_to_kill);
    while(1) { __asm__ volatile("hlt"); }
}

void yield() { task_scheduler(); }
void create_thread(void (*f)(void)) { create_process((void (*)(int, char**))f, 0, 0, "thread", kernel_dir); }
void task_sleep(int ms) { 
    if(!current_task) return; current_task->wake_tick = get_ticks() + ms;
    current_task->state = TASK_SLEEPING; task_scheduler(); 
}
void check_kill_flag() {
    if (current_task && current_task->kill_me) { current_task->kill_me = 0; printf("\n[Forced Exit]\n"); exit_process(); }
}

void wait_process(int pid) {
    foreground_task_id = pid; 
    while (1) {
        Task *t = ready_queue; int found = 0;
        do {
            if (t->id == pid && t->state != TASK_DEAD) { found = 1; break; }
            t = t->next;
        } while (t != ready_queue);
        if (!found) break;
        task_sleep(20);
    }
    foreground_task_id = current_task->id; 
}

void kill_task(int pid) {
    if (!ready_queue) return;
    Task *t = ready_queue; int found = 0;
    do {
        if (t->id == pid && t->state != TASK_DEAD) { found = 1; break; } t = t->next;
    } while (t != ready_queue);

    if (!found) return;
    if (t == current_task) { exit_process(); return; }
    if (t->id <= 1) { printf("Permission denied: cannot kill kernel/init.\n"); return; }
    if (t->uid == 0 && current_task->uid != 0) { printf("Permission denied: Only root can kill root processes.\n"); return; }

    kill_children_of(t->id);
    t->state = TASK_DEAD; tasks_need_cleanup = 1;
}

int spawn_process(char* path, char** argv, char* redirect_out) {
    int file_size = vfs_get_size(path);
    if (file_size <= 0) return -1;

    uint8_t *file_buf = (uint8_t*)kmalloc(file_size);
    vfs_read(path, file_buf);

    page_directory_t *app_pd = clone_page_directory();
    uint32_t entry_point = 0x40000000;
    uint32_t phys_addr_to_track = 0;

    Elf32_Ehdr *hdr = (Elf32_Ehdr*)file_buf;

    
    if (*(uint32_t*)hdr->e_ident == ELF_MAGIC) {
        entry_point = hdr->e_entry;
        Elf32_Phdr *phdrs = (Elf32_Phdr*)(file_buf + hdr->e_phoff);
        
        for (int i = 0; i < hdr->e_phnum; i++) {
            if (phdrs[i].p_type == PT_LOAD) {
                uint32_t mem_sz = phdrs[i].p_memsz;
                uint32_t file_sz = phdrs[i].p_filesz;
                uint32_t vaddr = phdrs[i].p_vaddr;

                uint32_t alloc_size = (mem_sz + 4095) & ~4095;
                uint32_t phys_addr = (uint32_t)kmalloc_a(alloc_size);
                if(phys_addr_to_track == 0) phys_addr_to_track = phys_addr; 
                
                fast_memset((void*)phys_addr, 0, alloc_size / 4);
                fast_memcpy((void*)phys_addr, file_buf + phdrs[i].p_offset, file_sz);

                for (uint32_t j = 0; j < alloc_size; j += 4096) {
                    paging_map_user(app_pd, phys_addr + j, (vaddr & 0xFFFFF000) + j, 7);
                }
            }
        }
    } else {
        
        uint32_t alloc_size = (file_size + 1024 * 1024 + 4095) & ~4095; 
        uint32_t phys_addr = (uint32_t)kmalloc_a(alloc_size);
        phys_addr_to_track = phys_addr;
        fast_memset((void*)phys_addr, 0, alloc_size / 4);
        fast_memcpy((void*)phys_addr, file_buf, file_size);

        for(uint32_t i = 0; i < alloc_size; i += 4096) {
            paging_map_user(app_pd, phys_addr + i, 0x40000000 + i, 7);
        }
    }

    kfree(file_buf);

    int argc = 0; if (argv) { while(argv[argc]) argc++; }
    char* name = path; for(int i = 0; path[i]; i++) if(path[i] == '/') name = &path[i+1];

    current_spawn_redirect = redirect_out; 
    int pid = create_process((void (*)(int, char**))entry_point, argc, argv, name, app_pd);
    
    Task *t = ready_queue;
    do {
        if (t->id == pid) { 
            t->app_phys_addr = phys_addr_to_track; 
            t->lib_offset = 0x50000000; 
            break; 
        }
        t = t->next;
    } while (t != ready_queue);

    return pid;
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
            track_allocation(t, (void*)phys_addr); 
            
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