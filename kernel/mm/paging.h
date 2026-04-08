#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include "idt/idt.h"
#include "irq/interrupts.h"

extern void _loadPageDirectory(uint32_t*);
extern void _enablePaging(void);

typedef struct registers {
    uint32_t ds; 
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};

typedef struct page_directory_t{
    uint32_t entries[1024];
} page_directory_t;

typedef struct page_table_t{
    uint32_t entries[1024];
} page_table_t;

extern page_directory_t *kernel_dir;
extern page_directory_t *current_dir;

void init_paging();

void paging_map(uint32_t phys, uint32_t virt, uint32_t flags);
void switch_page_directory(page_directory_t *dir);
void paging_map_user(page_directory_t *dir, uint32_t phys, uint32_t virt, uint32_t flags);
page_directory_t* clone_page_directory();
void paging_map_4mb(uint32_t phys, uint32_t virt, uint32_t flags);

#endif