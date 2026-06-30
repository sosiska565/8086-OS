/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/lib/stdint.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#ifndef _STDINT_H
#define _STDINT_H

#include "oslib.h"


#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif


#define UINT32_MAX 0xFFFFFFFF
#define INT32_MAX  0x7FFFFFFF
#define UINT16_MAX 0xFFFF
#define INT16_MAX  0x7FFF
#define UINT8_MAX  0xFF
#define INT8_MAX   0x7F

#endif
