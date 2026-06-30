/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/fs/cache.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include "fs/cache.h"
#include "mm/memory.h"
#include "drivers/timer/timer.h"
#include "utils/utils.h"

#define CACHE_SIZE 2048 

typedef struct {
    int drive_id;
    uint32_t lba;
    uint8_t data[512];
    uint32_t last_access;
    int valid;
} CacheEntry;

CacheEntry *disk_cache = 0;

void init_disk_cache(void) {
    disk_cache = (CacheEntry*)kmalloc(sizeof(CacheEntry) * CACHE_SIZE);
    for(int i = 0; i < CACHE_SIZE; i++) {
        disk_cache[i].valid = 0;
    }
}

int cache_read(int drive_id, uint32_t lba, uint8_t *buffer) {
    if(!disk_cache) return 0;
    for(int i = 0; i < CACHE_SIZE; i++) {
        if(disk_cache[i].valid && disk_cache[i].drive_id == drive_id && disk_cache[i].lba == lba) {
            disk_cache[i].last_access = get_ticks();
            fast_memcpy(buffer, disk_cache[i].data, 512);
            return 1; 
        }
    }
    return 0;
}

void cache_write_thru(int drive_id, uint32_t lba, uint8_t *buffer) {
    if(!disk_cache) return;
    int target_idx = 0;
    uint32_t min_tick = 0xFFFFFFFF;
    
    for(int i = 0; i < CACHE_SIZE; i++) {
        if(!disk_cache[i].valid) { target_idx = i; break; }
        if(disk_cache[i].drive_id == drive_id && disk_cache[i].lba == lba) { target_idx = i; break; }
        if(disk_cache[i].last_access < min_tick) { min_tick = disk_cache[i].last_access; target_idx = i; }
    }
    
    disk_cache[target_idx].valid = 1;
    disk_cache[target_idx].drive_id = drive_id;
    disk_cache[target_idx].lba = lba;
    disk_cache[target_idx].last_access = get_ticks();
    fast_memcpy(disk_cache[target_idx].data, buffer, 512);
}
