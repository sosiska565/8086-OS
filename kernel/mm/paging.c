#include "mm/paging.h"
#include "mm/memory.h"
#include "drivers/vga/vga.h"
#include "drivers/video/vesa.h"
#include "multiboot.h"
#include "system_apps/console/system.h"
#include "fs/fat/fat32.h"
#include "drivers/video/bga/gfx_console.h"

page_directory_t *kernel_dir = 0;
page_directory_t *current_dir = 0;

void paging_map(uint32_t phys, uint32_t virt, uint32_t flags) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x03FF;

    uint32_t *pd_entry = &kernel_dir->entries[pd_idx];
    
    page_table_t *table;

    if ((*pd_entry & 1) == 0) {
        table = (page_table_t*)kmalloc_a(sizeof(page_table_t));
        
        uint32_t *t_ptr = (uint32_t*)table;
        for(int i=0; i<1024; i++) t_ptr[i] = 0;

        *pd_entry = (uint32_t)table | 0x7; 
    } else {
        table = (page_table_t*) (*pd_entry & 0xFFFFF000);
    }

    table->entries[pt_idx] = (phys & 0xFFFFF000) | (flags | 1);
}

void paging_map_4mb(uint32_t phys, uint32_t virt, uint32_t flags) {
    uint32_t pd_idx = virt >> 22;
    kernel_dir->entries[pd_idx] = (phys & 0xFFC00000) | 0x80 | flags | 1; 
}

void init_paging() {
    printf("Initializing Paging...\n");

    kernel_dir = (page_directory_t*)kmalloc_a(sizeof(page_directory_t));
    
    for(int i=0; i<1024; i++) {
        kernel_dir->entries[i] = 0 | 2;
    }
    
    
    
    for(uint32_t i = 0; i < 0x20400000; i += 0x400000) {
        paging_map_4mb(i, i, 3);
    }
    
    extern uint32_t *video_memory; 
    extern int buffer_size_bytes;
    
    uint32_t vesa_addr = (uint32_t)video_memory;
    uint32_t vesa_size = (buffer_size_bytes + 4095) & ~4095; 

    printf("Mapping VESA LFB at 0x%x using 4MB Huge Pages...\n", vesa_addr);

    uint32_t vesa_aligned = vesa_addr & 0xFFC00000;
    uint32_t vesa_end = (vesa_addr + vesa_size + 0x3FFFFF) & 0xFFC00000;
    
    for(uint32_t addr = vesa_aligned; addr < vesa_end; addr += 0x400000) {
        paging_map_4mb(addr, addr, 3);
    }

    printf("Enabling Paging...\n");
    switch_page_directory(kernel_dir);
    _enablePaging();
    
    printf("Paging Enabled!\n");
}

void switch_page_directory(page_directory_t *dir) {
    current_dir = dir;
    _loadPageDirectory((uint32_t*)dir->entries);
}

page_directory_t* clone_page_directory() {
    page_directory_t* dir = (page_directory_t*)kmalloc_a(sizeof(page_directory_t));
    for(int i = 0; i < 1024; i++) {
        dir->entries[i] = kernel_dir->entries[i];
    }
    return dir;
}

void paging_map_user(page_directory_t *dir, uint32_t phys, uint32_t virt, uint32_t flags) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x03FF;

    uint32_t *pd_entry = &dir->entries[pd_idx];
    page_table_t *table;

    if ((*pd_entry & 1) == 0) {
        table = (page_table_t*)kmalloc_a(sizeof(page_table_t));
        uint32_t *t_ptr = (uint32_t*)table;
        for(int i=0; i<1024; i++) t_ptr[i] = 0;
        *pd_entry = (uint32_t)table | 0x7; 
    } else {
        table = (page_table_t*)(*pd_entry & 0xFFFFF000);
    }

    table->entries[pt_idx] = (phys & 0xFFFFF000) | (flags | 1);
}

