#include "memory/memory.h"
#include "drivers/vga/vga.h"

#define HEAP_MAGIC 0xAA55AA55

typedef struct memory_block {
    uint32_t magic;
    size_t size;
    int is_free;
    struct memory_block *next;
    struct memory_block *prev;
} memory_block_t;

static memory_block_t *heap_start = NULL;

void heap_init(void){
    printf("Initializing smart heap...\n");

    heap_start = (memory_block_t*)HEAP_START;
    heap_start->magic = HEAP_MAGIC;
    heap_start->size = HEAP_SIZE - sizeof(memory_block_t);
    heap_start->is_free = 1;
    heap_start->next = NULL;
    heap_start->prev = NULL;

    printf("Heap initialized at %X\n", (unsigned int)HEAP_START);
    printf("Heap size: %d KB\n", HEAP_SIZE / 1024);
}

void* kmalloc(size_t size){
    uint32_t flags = save_flags();
    if(size % 4 != 0) size = size + (4 - size % 4);

    memory_block_t *current = heap_start;

    while (current != NULL) {
        if(current->is_free && current->size >= size){
            if(current->size > size + sizeof(memory_block_t) + 4) {
                memory_block_t *new_block = (memory_block_t*)((uint8_t*)current + sizeof(memory_block_t) + size);
                new_block->magic = HEAP_MAGIC;
                new_block->size = current->size - size - sizeof(memory_block_t);
                new_block->is_free = 1;
                new_block->next = current->next;
                new_block->prev = current;

                if(current->next) current->next->prev = new_block;
                
                current->size = size;
                current->next = new_block;
            }
            current->is_free = 0;
            restore_flags(flags);
            return (void*)(current + 1);
        }
        current = current->next;
    }

    restore_flags(flags);
    return NULL;
}

void kfree(void* ptr){
    uint32_t flags = save_flags();
    if(ptr == NULL){
        restore_flags(flags);
        return;
    }

    memory_block_t *block = (memory_block_t*)ptr - 1;
    
    if(block->magic != HEAP_MAGIC) {
        restore_flags(flags);
        return; 
    }

    block->is_free = 1;

    if(block->next != NULL && block->next->is_free) {
        block->size += sizeof(memory_block_t) + block->next->size;
        block->next = block->next->next;
        if(block->next) block->next->prev = block;
    }

    if(block->prev != NULL && block->prev->is_free) {
        block->prev->size += sizeof(memory_block_t) + block->size;
        block->prev->next = block->next;
        if(block->next) block->next->prev = block->prev;
    }

    restore_flags(flags);
}

void* kmalloc_a(size_t size) {
    void* raw = kmalloc(size + 4096 + sizeof(void*));
    if (!raw) return NULL;
    uint32_t raw_addr = (uint32_t)raw;
    uint32_t aligned = (raw_addr + sizeof(void*) + 4095) & ~4095;
    void** orig_loc = (void**)(aligned - sizeof(void*));
    *orig_loc = raw;
    return (void*)aligned;
}

void kfree_a(void* ptr) {
    if (!ptr) return;
    void** orig_loc = (void**)((uint32_t)ptr - sizeof(void*));
    kfree(*orig_loc);
}

void heap_dump(void) {
    printf("\n=== HEAP DUMP ===\n");
    memory_block_t *current = heap_start;
    int block_num = 0;
    while(current != NULL) {
        printf("Block %d: ", block_num);
        if(current->is_free) printf("%s%C", "FREE", VGA_COLOR(VGA_COLOR_GREEN, VGA_COLOR_BLACK));
        else printf("%s%C", "USED", VGA_COLOR(VGA_COLOR_RED, VGA_COLOR_BLACK));
        
        printf("  Size: %d bytes  Addr: %x\n", current->size, (uint32_t)current);
        current = current->next;
        block_num++;
    }
    printf("=======================\n\n");
}

void fast_memset(void* dest, uint32_t val, size_t count_pixels) {
    __asm__ volatile (
        "cld\n"
        "rep stosl"
        : 
        : "a"(val), "D"(dest), "c"(count_pixels) 
        : "memory"
    );
}