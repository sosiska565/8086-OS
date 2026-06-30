/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/lib/stdarg.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include "oslib.h"
#ifndef va_copy
#define va_copy(d,s) __builtin_va_copy(d,s)
#endif
