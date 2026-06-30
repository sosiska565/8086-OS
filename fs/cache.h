/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/fs/cache.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#ifndef CACHE_H
#define CACHE_H

#include <stdint.h>

void init_disk_cache(void);
int cache_read(int drive_id, uint32_t lba, uint8_t *buffer);
void cache_write_thru(int drive_id, uint32_t lba, uint8_t *buffer);

#endif
