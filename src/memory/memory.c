#include "memory/memory.h"

typedef struct memory_block {
    size_t size;
    int is_free;
    struct memory_block *next;
} memory_block_t;

static memory_block_t *heap_start = NULL;

void heap_init(void){
    print("Initializing heap...\n");

    heap_start = (memory_block_t*)HEAP_START;

    heap_start->size = HEAP_SIZE - sizeof(memory_block_t);
    heap_start->is_free = 1;
    heap_start->next = NULL;

    print("Heap initialized at ");
    printhex((unsigned int)HEAP_START);
    print("\n");

    print("Heap size: ");
    printnumber(HEAP_SIZE / 1024);
    print(" KB\n");
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
    print("\n=== HEAP DUMP ===\n");
    
    memory_block_t *current = heap_start;
    int block_num = 0;
    
    while(current != NULL) {
        print("Block ");
        printnumber(block_num);
        print(": ");
        
        if(current->is_free) {
            print_colored("FREE", VGA_COLOR(VGA_COLOR_GREEN, VGA_COLOR_BLACK));
        } else {
            print_colored("USED", VGA_COLOR(VGA_COLOR_RED, VGA_COLOR_BLACK));
        }
        
        print("  Size: ");
        printnumber(current->size);
        print(" bytes");
        
        print("  Addr: ");
        printhex((uint32_t)current);
        print("\n");
        
        current = current->next;
        block_num++;
    }
    
    print("=================\n\n");
}