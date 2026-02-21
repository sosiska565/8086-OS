#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

#define HEAP_START 0x00200000
#define HEAP_SIZE 0x20000000

static inline uint32_t save_flags() {
    uint32_t flags;
    __asm__ volatile(
        "pushfl\n\t"
        "popl %0"
        : "=r"(flags)
        :
        : "memory"
    );
    
    __asm__ volatile("cli");
    
    return flags;
}

static inline void restore_flags(uint32_t flags) {
    __asm__ volatile(
        "pushl %0\n\t"
        "popfl"
        :
        : "r"(flags)
        : "memory", "cc"
    );
}

void heap_init(void);
void* kmalloc(size_t size);
void kfree(void *ptr);
void heap_dump(void);
void* kmalloc_a(size_t size);
void kfree_a(void *ptr);  
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

static inline void fast_memcpy(void *dest, const void* src, uint32_t n){
    uint32_t dwords = n / 4;
    uint32_t bytes = n % 4;
    __asm__ volatile(
        "rep movsl\n\t"
        "mov %3, %%ecx\n\t"
        "rep movsb"
        : "+D"(dest), "+S"(src), "+c"(dwords)
        : "r"(bytes)
        : "memory"
    );
}

void fast_memset(void* dest, uint32_t val, size_t count_pixels);

#endif