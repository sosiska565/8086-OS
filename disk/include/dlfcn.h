/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/lib/dlfcn.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include "oslib.h"
#define RTLD_DEFAULT ((void *)0)
#define RTLD_GLOBAL 0
#define RTLD_LAZY 0
int dlclose(void *handle);
