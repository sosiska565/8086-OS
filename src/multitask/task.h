#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include "drivers/video/graphics.h"
#include "memory/paging.h"

typedef struct {
    int id;
    int parent_id;
    int state;
    char name[32];
} task_info_t;

typedef enum {
    TASK_RUNNING,
    TASK_READY,
    TASK_SLEEPING,
    TASK_DEAD
} TaskState;

typedef struct AllocList {
    void *ptr;
    struct AllocList *next;
} AllocList;

typedef struct Task{
    int id;
    uint32_t esp;
    uint32_t stack_start;
    struct Task *next;
    Window *window;
    TaskState state;
    int owns_window;
    int kill_me;
    uint32_t wake_tick;
    int parent_id;
    AllocList *allocations;
    char name[32];
    struct page_directory_t *page_dir;
    uint32_t app_phys_addr;
} Task;

typedef struct process_struct{
    void (*foo)(int, char**);
    int argc;
    char **argv;
    char *name;
} process_struct;

extern Task *current_task;
extern Task *ready_queue;
void check_kill_flag();

void init_tasking();
void cleanup_zombies();
void create_thread(void (*function)(void));
void task_scheduler();
void yield();
void task_exit();
int create_process(void (*entry)(int, char**), int argc, char **argv, char *name, struct page_directory_t *pd);
void exit_process();
void wait_process(int pid);
void kill_focused_process();
void task_sleep(int ms);
void kill_task(int pid);
void track_allocation(Task *task, void *ptr);
void untrack_allocation(Task *task, void *ptr);
Task* get_task_by_window(Window *win);
Task* get_focused_task();

#endif