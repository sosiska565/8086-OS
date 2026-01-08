#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

#define HEAP_START 0x00200000
#define HEAP_SIZE 0x20000000

void heap_init(void);
void* kmalloc(size_t size);
void kfree(void *ptr);
void heap_dump(void);

#endif