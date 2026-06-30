/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/lib/strings.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#ifndef _STRINGS_H
#define _STRINGS_H

#include "oslib.h"

static inline int strcasecmp(const char *s1, const char *s2) {
    while (1) {
        char c1 = (*s1 >= 'A' && *s1 <= 'Z') ? *s1 + 32 : *s1;
        char c2 = (*s2 >= 'A' && *s2 <= 'Z') ? *s2 + 32 : *s2;
        if (c1 != c2 || c1 == '\0') return c1 - c2;
        s1++; s2++;
    }
}

static inline int strncasecmp(const char *s1, const char *s2, size_t n) {
    if (n == 0) return 0;
    do {
        char c1 = (*s1 >= 'A' && *s1 <= 'Z') ? *s1 + 32 : *s1;
        char c2 = (*s2 >= 'A' && *s2 <= 'Z') ? *s2 + 32 : *s2;
        if (c1 != c2 || c1 == '\0') return c1 - c2;
        s1++; s2++;
    } while (--n > 0);
    return 0;
}

#endif
