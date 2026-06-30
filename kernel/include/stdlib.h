/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/kernel/include/stdlib.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#ifndef _KERNEL_STDLIB_H
#define _KERNEL_STDLIB_H



long atoi(const char *str, int base);




#define atoi(str) (int)atoi((str), 10)

#endif
