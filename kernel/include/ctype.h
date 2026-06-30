/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/kernel/include/ctype.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#ifndef _KERNEL_CTYPE_H
#define _KERNEL_CTYPE_H


char toupper_char(unsigned int c);

#define toupper(c) (int)toupper_char((unsigned int)(c))


unsigned int tolower(unsigned int c);


static inline int isspace(int c) { 
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f'; 
}
static inline int isdigit(int c) { return c >= '0' && c <= '9'; }
static inline int isalpha(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static inline int isalnum(int c) { return isalpha(c) || isdigit(c); }
static inline int islower(int c) { return c >= 'a' && c <= 'z'; }
static inline int isupper(int c) { return c >= 'A' && c <= 'Z'; }
static inline int isprint(int c) { return c >= 0x20 && c <= 0x7E; }
static inline int isxdigit(int c) { return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }

#endif
