#include "memory/memory.h"
#include "drivers/vga/vga.h"

#define HEAP_MAGIC 0xAA55AA55
#define NUM_BINS 16

typedef struct memory_block {
    uint32_t magic;
    size_t size;
    int is_free;
    struct memory_block *next;
    struct memory_block *prev;
    struct memory_block *free_next;
    struct memory_block *free_prev;  
} memory_block_t;

static memory_block_t *free_bins[NUM_BINS];
static memory_block_t *heap_start = NULL;

static inline int get_bin_index(size_t size) {
    if (size <= 32) return 0;
    if (size <= 64) return 1;
    if (size <= 128) return 2;
    if (size <= 256) return 3;
    if (size <= 512) return 4;
    if (size <= 1024) return 5;
    if (size <= 2048) return 6;
    if (size <= 4096) return 7;
    if (size <= 8192) return 8;
    if (size <= 16384) return 9;
    if (size <= 32768) return 10;
    if (size <= 65536) return 11;
    if (size <= 131072) return 12;
    if (size <= 262144) return 13;
    if (size <= 524288) return 14;
    return 15;
}

static void insert_free_block(memory_block_t *block) {
    int bin = get_bin_index(block->size);
    block->free_next = free_bins[bin];
    block->free_prev = NULL;
    if (free_bins[bin]) free_bins[bin]->free_prev = block;
    free_bins[bin] = block;
}

static void remove_free_block(memory_block_t *block) {
    int bin = get_bin_index(block->size);
    if (block->free_prev) block->free_prev->free_next = block->free_next;
    else free_bins[bin] = block->free_next;
    
    if (block->free_next) block->free_next->free_prev = block->free_prev;
}

void heap_init(void){
    printf("Initializing smart heap...\n");

    for (int i = 0; i < NUM_BINS; i++) {
        free_bins[i] = NULL;
    }

    heap_start = (memory_block_t*)HEAP_START;
    heap_start->magic = HEAP_MAGIC;
    heap_start->size = HEAP_SIZE - sizeof(memory_block_t);
    heap_start->is_free = 1;
    heap_start->next = NULL;
    heap_start->prev = NULL;

    insert_free_block(heap_start);

    printf("Heap initialized at 0x%X\n", (unsigned int)HEAP_START);
    printf("Heap size: %d MB\n", HEAP_SIZE / (1024 * 1024));
}

void* kmalloc(size_t size){
    uint32_t flags = save_flags();
    
    if(size % 4 != 0) size = size + (4 - size % 4);

    int start_bin = get_bin_index(size);
    memory_block_t *found = NULL;

    for (int i = start_bin; i < NUM_BINS; i++) {
        memory_block_t *curr = free_bins[i];
        while (curr != NULL) {
            if (curr->size >= size) {
                found = curr;
                break;
            }
            curr = curr->free_next;
        }
        if (found) break;
    }

    if (!found) {
        restore_flags(flags);
        return NULL;
    }

    remove_free_block(found);

    if(found->size > size + sizeof(memory_block_t) + 16) {
        memory_block_t *new_block = (memory_block_t*)((uint8_t*)found + sizeof(memory_block_t) + size);
        new_block->magic = HEAP_MAGIC;
        new_block->size = found->size - size - sizeof(memory_block_t);
        new_block->is_free = 1;
        
        new_block->next = found->next;
        new_block->prev = found;
        if(found->next) found->next->prev = new_block;
        found->next = new_block;
        
        found->size = size;
        
        insert_free_block(new_block);
    }
    
    found->is_free = 0;
    restore_flags(flags);
    return (void*)(found + 1);
}

void kfree(void* ptr){
    uint32_t flags = save_flags();
    if(ptr == NULL){
        restore_flags(flags);
        return;
    }

    memory_block_t *block = (memory_block_t*)ptr - 1;
    
    if(block->magic != HEAP_MAGIC || block->is_free) {
        restore_flags(flags);
        return; 
    }

    if(block->next != NULL && block->next->is_free) {
        remove_free_block(block->next);
        block->size += sizeof(memory_block_t) + block->next->size;
        block->next = block->next->next;
        if(block->next) block->next->prev = block;
    }

    if(block->prev != NULL && block->prev->is_free) {
        remove_free_block(block->prev);
        block->prev->size += sizeof(memory_block_t) + block->size;
        block->prev->next = block->next;
        if(block->next) block->next->prev = block->prev;
        block = block->prev;
    }

    block->is_free = 1;
    insert_free_block(block);

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
    printf("\n%C=== DETAILED HEAP DUMP ===%C\n", VGA32_COLOR_YELLOW, VGA32_COLOR_WHITE);
    
    if (heap_start == NULL) {
        printf("Heap is not initialized yet.\n");
        return;
    }

    memory_block_t *curr = heap_start;
    int total_blocks = 0;
    int free_blocks = 0;
    int used_blocks = 0;
    size_t total_free_memory = 0;
    size_t total_used_memory = 0;

    printf("--- Physical Memory Blocks ---\n");
    
    while(curr != NULL) {
        printf("Addr: %X | Size: %d bytes | Status: ", (uint32_t)curr, curr->size);
        
        if(curr->is_free) {
            printf("%C[ FREE ]%C\n", VGA32_COLOR_GREEN, VGA32_COLOR_WHITE);
            free_blocks++;
            total_free_memory += curr->size;
        } else {
            printf("%C[ USED ]%C\n", VGA32_COLOR_RED, VGA32_COLOR_WHITE);
            used_blocks++;
            total_used_memory += curr->size;
        }
        
        curr = curr->next;
        total_blocks++;
        
        if(total_blocks > 50000) {
            printf("%C... Heap structure corrupted or too many blocks. Stopping dump.%C\n", 
                   VGA32_COLOR_RED, VGA32_COLOR_WHITE);
            break;
        }
    }

    printf("\n--- Free Bins Status (Segregated Lists) ---\n");
    for(int i = 0; i < NUM_BINS; i++) {
        int count = 0;
        memory_block_t *fb = free_bins[i];
        while(fb) { 
            count++; 
            fb = fb->free_next; 
        }
        if(count > 0) {
            int max_size = (i == 15) ? -1 : (32 << i);
            if (max_size == -1) {
                printf("Bin %d (> 512 KB): %d blocks\n", i, count);
            } else {
                printf("Bin %d (up to %d bytes): %d blocks\n", i, max_size, count);
            }
        }
    }

    printf("\n--- Heap Summary ---\n");
    printf("Total Blocks : %d\n", total_blocks);
    printf("Used Blocks  : %C%d%C\n", VGA32_COLOR_RED, used_blocks, VGA32_COLOR_WHITE);
    printf("Free Blocks  : %C%d%C\n", VGA32_COLOR_GREEN, free_blocks, VGA32_COLOR_WHITE);
    printf("Used Memory  : %d bytes (~%d KB)\n", total_used_memory, total_used_memory / 1024);
    printf("Free Memory  : %d bytes (~%d MB)\n", total_free_memory, total_free_memory / (1024 * 1024));
    printf("%C==========================%C\n\n", VGA32_COLOR_YELLOW, VGA32_COLOR_WHITE);
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

size_t get_used_memory(void) {
    size_t free_mem = 0;
    uint32_t flags = save_flags(); 
    
    for(int i = 0; i < 16; i++) {
        memory_block_t *curr = free_bins[i];
        while(curr) {
            free_mem += curr->size;
            curr = curr->free_next;
        }
    }
    
    restore_flags(flags);
    return HEAP_SIZE - free_mem;
}

size_t get_total_memory(void) {
    return HEAP_SIZE;
}