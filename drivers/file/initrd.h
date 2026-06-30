/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/drivers/file/initrd.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#ifndef INITRD_H
#define INITRD_H

#include "multiboot.h"

void initrd_files(struct multiboot_info* mbi);

#endif
