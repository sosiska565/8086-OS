#ifndef CACHE_H
#define CACHE_H

#include <stdint.h>

void init_disk_cache(void);
int cache_read(int drive_id, uint32_t lba, uint8_t *buffer);
void cache_write_thru(int drive_id, uint32_t lba, uint8_t *buffer);

#endif