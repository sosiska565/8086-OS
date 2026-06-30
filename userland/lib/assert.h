/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/lib/assert.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#ifndef _ASSERT_H
#define _ASSERT_H

#include "oslib.h"


#define assert(expr) \
    do { \
        if (!(expr)) { \
            set_color(COLOR_LIGHT_RED, COLOR_BLACK); \
            printf("ASSERTION FAILED: %s\nFile: %s\nLine: %d\n", #expr, __FILE__, __LINE__); \
            set_color(COLOR_WHITE, COLOR_BLACK); \
            while (1) { yield(); } \
        } \
    } while (0)

#endif
