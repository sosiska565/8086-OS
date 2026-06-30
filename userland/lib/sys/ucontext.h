/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/lib/sys/ucontext.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include "oslib.h"
#define REG_EBP 6
#define REG_EIP 14
typedef struct { int gregs[19]; } mcontext_t;
typedef struct { mcontext_t uc_mcontext; } ucontext_t;
