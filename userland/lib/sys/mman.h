/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/lib/sys/mman.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include "oslib.h"
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4
#define MAP_PRIVATE 2
#define MAP_ANONYMOUS 0x20
#define MAP_FAILED ((void *)-1)
void *mmap(void *a, size_t l, int p, int f, int d, int o);
int munmap(void *a, size_t l);
int mprotect(void *addr, size_t len, int prot);
