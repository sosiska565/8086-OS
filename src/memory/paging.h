#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include "interrupt/idt/idt.h"
#include "interrupt/interrupts/interrupts.h"

extern void _loadPageDirectory(uint32_t*);
extern void _enablePaging(void);

struct registers {
    uint32_t ds; 
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};

typedef struct{
    uint32_t entries[1024];
} page_directory_t;

typedef struct{
    uint32_t entries[1024];
} page_table_t;

void init_paging();

void paging_map(uint32_t phys, uint32_t virt, uint32_t flags);
void switch_page_directory(page_directory_t *dir);
void page_fault_handler_c(struct registers *reg);

#endif