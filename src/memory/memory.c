#include "memory/memory.h"
#include "drivers/vga/vga.h"

typedef struct memory_block {
    size_t size;
    int is_free;
    struct memory_block *next;
} memory_block_t;

static memory_block_t *heap_start = NULL;

void heap_init(void){
    printf("Initializing heap...\n");

    heap_start = (memory_block_t*)HEAP_START;

    heap_start->size = HEAP_SIZE - sizeof(memory_block_t);
    heap_start->is_free = 1;
    heap_start->next = NULL;

    printf("Heap initialized at ");
    printf("%X", (unsigned int)HEAP_START);
    printf("\n");

    printf("Heap size: ");
    printf("%d", HEAP_SIZE / 1024);
    printf(" KB\n");
}

void* kmalloc(size_t size){
    if(size % 4 != 0){
        size = size + (4 - size % 4);
    }

    memory_block_t *current = heap_start;

    while (current != NULL) {
        if(current->is_free && current->size >= size){
            if(current->size > size + sizeof(memory_block_t) + 4) {
                memory_block_t *new_block = (memory_block_t*)((uint8_t*)current + sizeof(memory_block_t) + size);

                new_block->size = current->size - size - sizeof(memory_block_t);
                new_block->is_free = 1;
                new_block->next = current->next;

                current->size = size;
                current->next = new_block;
            }

            current->is_free = 0;

            return (void*)(current + 1);
        }

        current = current->next;
    }

    printn("kmalloc: out of memory!\n");
    return NULL;
}

void kfree(void* ptr){
    if(ptr == NULL){
        return;
    }

    memory_block_t *block = (memory_block_t*)ptr - 1;

    block->is_free = 1;

    if(block->next != NULL && block->next->is_free) {
        block->size += sizeof(memory_block_t) + block->next->size;
        block->next = block->next->next;
    }

    memory_block_t *current = heap_start;
    while (current != NULL && current->next != block) {
        current = current->next;
    }
    
    if(current != NULL && current->is_free){
        current->size += sizeof(memory_block_t) + block->size;
        current->next = block->next;
    }
}

void heap_dump(void) {
    printf("\n=== HEAP DUMP ===\n");
    
    memory_block_t *current = heap_start;
    int block_num = 0;

    if (current == NULL) {
        printf("OUT OF MEMORY! Wanted: %d bytes\n", current->size);
        while(1);
    }
    
    while(current != NULL) {
        printf("Block ");
        printf("%d", block_num);
        printf(": ");
        
        if(current->is_free) {
            printf("%s%C", "FREE", VGA_COLOR(VGA_COLOR_GREEN, VGA_COLOR_BLACK));
        } else {
            printf("%s%C", "USED", VGA_COLOR(VGA_COLOR_RED, VGA_COLOR_BLACK));
        }
        
        printf("  Size: ");
        printf("%d", current->size);
        printf(" bytes");
        
        printf("  Addr: ");
        printf("%X", (uint32_t)current);
        printf("\n");
        
        current = current->next;
        block_num++;
    }
    
    printf("=================\n\n");
}

void fast_memcpy(void* dest, const void* src, size_t count_bytes) {
    size_t dwords = count_bytes / 4;
    
    __asm__ volatile (
        "cld\n"
        "rep movsl"
        : 
        : "S"(src), "D"(dest), "c"(dwords) 
        : "memory"
    );
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