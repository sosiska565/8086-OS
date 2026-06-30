/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/kernel/task/task.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include "mm/paging.h"

#define MAX_FDS 16 

#define SIGINT  2
#define SIGKILL 9
#define SIGSEGV 11
#define MAX_SIG 32

typedef struct {
    int id;
    int parent_id;
    int state;
    char name[32];
} task_info_t;

typedef enum { TASK_RUNNING, TASK_READY, TASK_SLEEPING, TASK_DEAD } TaskState;

typedef struct AllocList { void *ptr; struct AllocList *next; } AllocList;

typedef void (*sighandler_t)(int);

struct trap_frame {
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags;
} __attribute__((packed));

typedef struct Task {
    int id;
    uint32_t esp;
    uint32_t stack_start;
    struct Task *next;
    TaskState state;
    int kill_me;
    uint32_t wake_tick;
    int parent_id;
    AllocList *allocations;
    char name[32];
    struct page_directory_t *page_dir;
    uint32_t app_phys_addr;

    char cwd[64]; 
    int fd_table[MAX_FDS]; 

    int uid; 
    uint32_t lib_offset;
    uint32_t heap_start;
    uint32_t heap_end;
 
    uint32_t blocked_signals; 
    uint32_t pending_signals;
    uint32_t signal_handlers[MAX_SIG]; 
    struct trap_frame sig_saved_frame;
    int in_signal_handler;
} Task;

typedef struct process_struct {
    void (*foo)(int, char**);
    int argc;
    char **argv;
    char *name;
} process_struct;

extern Task *current_task;
extern Task *ready_queue;
extern int foreground_task_id; 

void check_kill_flag(void);
void init_tasking(void);
void cleanup_zombies(void);
void create_thread(void (*function)(void));
void task_scheduler(void);
void yield(void);
int create_process(void (*entry)(int, char**), int argc, char **argv, char *name, struct page_directory_t *pd, int fd_in, int fd_out);
void exit_process(void);
void wait_process(int pid);
void task_sleep(int ms);
void kill_task(int pid);

void track_allocation(Task *task, void *ptr);
void track_allocation_a(Task *task, void *ptr);
void untrack_allocation(Task *task, void *ptr);

int spawn_process(char* path, char** argv);
int spawn_process_ext(char* path, char** argv, int fd_in, int fd_out);
uint32_t load_library(Task* t, char* lib_name);
uint32_t get_symbol(Task* t, uint32_t lib_handle, char* sym_name);

#endif
