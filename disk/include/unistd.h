/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/lib/unistd.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include "oslib.h"
extern char **environ;
int unlink(const char *pathname);
int close(int fd);
int lseek(int fd, int offset, int whence);
int execvp(const char *file, char *const argv[]);
