/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/kernel/include/string.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#ifndef _KERNEL_STRING_H
#define _KERNEL_STRING_H

#include <stddef.h>
#include <stdint.h>


void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);


int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, int n);
int strlen(const char *s);
void strcpy(char *dst, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
void strcat(char *dest, const char *src);
char *strchr(const char *s, int c);

#endif
