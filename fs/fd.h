/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/fs/fd.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#ifndef FD_H
#define FD_H

#include <stdint.h>
#include "task/task.h"

#define FILE_TYPE_NORMAL 0
#define FILE_TYPE_PIPE   1
#define FILE_TYPE_SOCKET 2 
#define FILE_TYPE_SOCKET_TCP 3 

typedef struct {
    uint8_t buffer[4096];
    int head;
    int tail;
    int count;
    int readers;
    int writers;
} PipeData;

typedef struct {
    int in_use;
    int refcount;
    int type; 
    
    char path[256];
    uint8_t *buffer;
    uint32_t size;
    uint32_t offset;
    uint32_t capacity;
    int mode;
    int is_dirty;
    
    PipeData *pipe;
    int socket_id; 
} KFile;

#define MAX_OPEN_FILES 256
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#define O_CREAT  0x0200
#define O_TRUNC  0x0400

extern KFile system_open_files[MAX_OPEN_FILES];

void fd_init(void);
void fd_inherit(Task *child, Task *parent);
int sys_open(char *path, int flags);
int sys_close(int fd);
int sys_read_fd(int fd, uint8_t *buf, uint32_t count);
int sys_write_fd(int fd, uint8_t *buf, uint32_t count);
int sys_lseek(int fd, int offset, int whence);
int sys_pipe_impl(int *fds);

#endif
