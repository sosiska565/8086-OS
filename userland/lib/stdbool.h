/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/lib/stdbool.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#ifndef _STDBOOL_H
#define _STDBOOL_H
#define bool _Bool
#ifndef DOOM_PORT
#define true 1
#define false 0
#endif
#endif
