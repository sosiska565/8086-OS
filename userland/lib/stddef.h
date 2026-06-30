/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/lib/stddef.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include "oslib.h"
#define offsetof(type, member) __builtin_offsetof(type, member)
