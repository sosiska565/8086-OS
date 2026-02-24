#include "memory/paging.h"
#include "memory/memory.h"
#include "drivers/vga/vga.h"
#include "drivers/video/vesa.h"
#include "multiboot.h"
#include "programs/system/console/system.h"
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

void init_paging() {
    printf("Initializing Paging...\n");

    kernel_dir = (page_directory_t*)kmalloc_a(sizeof(page_directory_t));
    
    for(int i=0; i<1024; i++) {
        kernel_dir->entries[i] = 0 | 2; // RW
    }
    
    uint32_t i = 0;
    for(i = 0; i < 0x20200000; i += 4096) {
        paging_map(i, i, 3);
    }
    extern uint32_t *video_memory; 
    extern int screen_width, screen_height;
    
    uint32_t vesa_addr = (uint32_t)video_memory;
    uint32_t vesa_size = 1024 * 1024 * 32;

    printf("Mapping VESA LFB at 0x%x (size %d)...\n", vesa_addr, vesa_size);

    for(i = 0; i < vesa_size; i += 4096) {
        paging_map(vesa_addr + i, vesa_addr + i, 3);
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
        *pd_entry = (uint32_t)table | 0x7; // Present, RW, User
    } else {
        table = (page_table_t*)(*pd_entry & 0xFFFFF000);
    }

    table->entries[pt_idx] = (phys & 0xFFFFF000) | (flags | 1);
}