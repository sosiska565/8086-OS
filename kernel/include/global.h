/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/kernel/include/global.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdint.h>
#include "kernel/include/version.h"

extern int $;
extern unsigned short isReadMode;
extern char* path;

extern char wallpaper_path[128];
extern uint32_t *wallpaper_buf;
extern int font_scale;

#endif
