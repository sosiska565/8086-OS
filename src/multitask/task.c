#include "multitask/task.h"
#include "memory/memory.h"
#include "drivers/vga/vga.h"
#include "drivers/video/graphics.h"
#include "drivers/io/io.h"
#include "drivers/timer/timer.h"

extern void switch_to_task(Task *next, Task *prev);

Task *current_task = 0;
Task *ready_queue = 0;
int next_pid = 0;

int kill_current_task_flag = 0;

Task *zombie_task = 0; 

void track_allocation(Task *task, void *ptr) {
    if (!task || !ptr) return;
    
    AllocList *node = (AllocList*)kmalloc(sizeof(AllocList));
    node->ptr = ptr;
    node->next = task->allocations;
    task->allocations = node;
}


void untrack_allocation(Task *task, void *ptr) {
    if (!task || !task->allocations) return;
    
    AllocList *curr = task->allocations;
    AllocList *prev = 0;
    
    while (curr) {
        if (curr->ptr == ptr) {
            if (prev) prev->next = curr->next;
            else task->allocations = curr->next;
            
            kfree(curr); 
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}


void free_task_memory(Task *task) {
    AllocList *curr = task->allocations;
    while (curr) {
        AllocList *temp = curr;
        
        kfree(curr->ptr); 
        curr = curr->next;
        
        kfree(temp);
    }
    task->allocations = 0;
}




void free_task_struct(Task *t) {
    if (!t) return;
    free_task_memory(t);
    if (t->stack_start) kfree((void*)t->stack_start);
    kfree(t);
}

void cleanup_zombies() {
    __asm__ volatile("cli");
    if (zombie_task != 0) {
        free_task_struct(zombie_task);
        zombie_task = 0;
    }
    __asm__ volatile("sti");
}

void task_set_name(Task* t, char* name) {
    int i = 0;
    if (!name) {
        t->name[0] = 'u'; t->name[1] = 'n'; t->name[2] = 'k'; t->name[3] = 0;
        return;
    }
    while (name[i] && i < 31) {
        t->name[i] = name[i];
        i++;
    }
    t->name[i] = 0;
}

void init_tasking() {
    Task *ktask = (Task*)kmalloc(sizeof(Task));
    ktask->id = next_pid++;
    ktask->esp = 0; 
    ktask->stack_start = 0; 
    ktask->next = ktask; 
    ktask->state = TASK_RUNNING;
    ktask->window = 0;
    ktask->owns_window = 0;
    ktask->parent_id = -1;
    ktask->allocations = 0; 

    task_set_name(ktask, "kernel");

    current_task = ktask;
    ready_queue = ktask;

    __asm__ volatile ("sti");
}

int create_process(void (*entry)(int, char**), int argc, char **argv, char *name) {
    cleanup_zombies();

    Task *new_task = (Task*)kmalloc(sizeof(Task));
    new_task->id = next_pid++;
    new_task->state = TASK_READY;
    new_task->kill_me = 0;
    new_task->allocations = 0;

    task_set_name(new_task, name);

    if (current_task) {
        new_task->parent_id = current_task->id;
        new_task->window = current_task->window;
    } else {
        new_task->parent_id = -1;
        new_task->window = 0;
    }
    new_task->owns_window = 0;

    uint32_t *stack = (uint32_t*)kmalloc(8192);
    new_task->stack_start = (uint32_t)stack;

    uint32_t *esp = stack + 2048;
    extern void exit_process(); 

    *(--esp) = (uint32_t)argv;
    *(--esp) = (uint32_t)argc;
    *(--esp) = (uint32_t)exit_process;
    *(--esp) = (uint32_t)entry;
    
    
    for(int i = 0; i < 8; i++) *(--esp) = 0;
    *(--esp) = 0x202;

    new_task->esp = (uint32_t)esp;

    __asm__ volatile("cli");
    Task *temp = ready_queue->next;
    ready_queue->next = new_task;
    new_task->next = temp;
    __asm__ volatile("sti");

    return new_task->id;
}


void task_scheduler() {
    cleanup_zombies();
    if (!current_task || !ready_queue) return;

    Task *prev = current_task;
    Task *next = prev->next;

    while (1) {
        if (next->state == TASK_RUNNING || next->state == TASK_READY) break;

        if (next->state == TASK_SLEEPING) {
            if (get_ticks() >= next->wake_tick) {
                next->state = TASK_RUNNING;
                break;
            }
        }
        
        next = next->next;
        if (next == prev) {
            if (prev->state == TASK_RUNNING) return;
            break; 
        }
    }

    if (next == prev) return;

    current_task = next;
    switch_to_task(next, prev);
}




void kill_children_of(int pid) {
    if (!ready_queue) return;
    Task *t = ready_queue;
    do {
        if (t->parent_id == pid && t->state != TASK_DEAD) {
            kill_children_of(t->id); 
            if (t->window && t->owns_window) wm_close_window(t->window);
            
            t->state = TASK_DEAD; 
        }
        t = t->next;
    } while (t != ready_queue);
}

void exit_process() {
    __asm__ volatile("cli");
    kill_current_task_flag = 0;
    outb(0x20, 0x20);

    kill_children_of(current_task->id);

    Task *task_to_kill = current_task;
    
    
    Task *prev = ready_queue;
    while (prev->next != task_to_kill) { prev = prev->next; }
    prev->next = task_to_kill->next;
    if (ready_queue == task_to_kill) ready_queue = task_to_kill->next;

    
    if (task_to_kill->window && task_to_kill->owns_window) {
        wm_close_window(task_to_kill->window);
    }

    task_to_kill->state = TASK_DEAD;
    
    zombie_task = task_to_kill;

    Task *next_live = task_to_kill->next;
    current_task = next_live;
    
    switch_to_task(next_live, task_to_kill);
    while(1) { __asm__ volatile("hlt"); }
}

void yield() { task_scheduler(); }
void create_thread(void (*f)(void)) { create_process((void (*)(int, char**))f, 0, 0, "thread"); }
void task_sleep(int ms) { 
    if(!current_task) return;
    current_task->wake_tick = get_ticks() + ms;
    current_task->state = TASK_SLEEPING;
    task_scheduler(); 
}
void kill_focused_process() {
    
    if (focused_window == 0) return;
    __asm__ volatile("cli");
    Task *t = ready_queue;
    int found = 0;
    do {
        if (t->window == focused_window && t->owns_window) { found = 1; break; }
        t = t->next;
    } while (t != ready_queue);

    if (found) {
        if (t == current_task) kill_current_task_flag = 1;
        else {
            kill_children_of(t->id);
            t->state = TASK_DEAD; 
            if (t->window) wm_close_window(t->window);
        }
    }
    __asm__ volatile("sti");
}
void check_kill_flag() {
    if (kill_current_task_flag) {
        kill_current_task_flag = 0;
        printf("\n[Forced Exit]\n");
        outb(0x20, 0x20); 
        exit_process();
    }
}


void wait_process(int pid) {
    while (1) {
        Task *t = ready_queue;
        int found = 0;

        do {
            if (t->id == pid && t->state != TASK_DEAD) {
                found = 1;
                break;
            }
            t = t->next;
        } while (t != ready_queue);

        if (!found) break;

        task_sleep(20);
    }
}


void kill_task(int pid) {
    if (!ready_queue) return;

    Task *t = ready_queue;
    int found = 0;

    do {
        if (t->id == pid && t->state != TASK_DEAD) {
            found = 1;
            break;
        }
        t = t->next;
    } while (t != ready_queue);

    if (!found) {
        return;
    }
    
    if (t == current_task) {
        printf("Use 'exit' to stop current process.\n");
        return;
    }
    
    if (t->id <= 1) { 
        printf("Permission denied: Cannot kill system process.\n");
        return;
    }

    kill_children_of(t->id);

    if (t->window && t->owns_window) {
        wm_close_window(t->window);
        t->window = 0;
    }

    t->state = TASK_DEAD;
}

Task* get_task_by_window(Window *win) {
    if (!ready_queue || !win) return 0;

    Task *t = ready_queue;
    do {
        if (t->window == win && t->state != TASK_DEAD) {
            return t;
        }
        t = t->next;
    } while (t != ready_queue);

    return 0;
}

Task* get_focused_task() {
    if (focused_window == 0) return 0;
    return get_task_by_window(focused_window);
}