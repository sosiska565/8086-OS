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
static inline void memset32(void *addr, uint32_t val, uint32_t count) {
    uint32_t *d = (uint32_t*)addr;
    __asm__ volatile(
        "cld\n"
        "rep stosl"
        : "+D" (d), "+c"(count)
        : "a"(val)
        : "memory"
    );
}

void fast_memcpy(void* dest, const void* src, size_t count_bytes);
void fast_memset(void* dest, uint32_t val, size_t count_pixels);

#endif