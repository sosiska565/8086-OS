/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/lib/semaphore.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include "oslib.h"
typedef int sem_t;
#define sem_init(s, p, v) (*(s)=(v),0)
#define sem_wait(s) (0)
#define sem_post(s) (0)
