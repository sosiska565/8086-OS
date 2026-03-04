#include "fs/fat/fat32.h"
#include "drivers/file/ATA/ATA.h"
#include "drivers/vga/vga.h"
#include "global.h"
#include "memory/memory.h" 

struct fat32_bpb bpb;
uint32_t fat_start_sector;
uint32_t data_start_sector;


static uint8_t fat_dma_buffer[512] __attribute__((aligned(4096)));

void to_dos_file_name(char* input, char* output) {
    for(int k=0; k<11; k++) output[k] = ' ';
    int i = 0, j = 0;
    while(input[i] != '\0' && input[i] != '.' && j < 8) {
        output[j] = toupper_char((unsigned int)input[i]);
        i++; j++;
    }
    if(input[i] == '.') {
        i++; j = 8;
        while(input[i] != '\0' && j < 11) {
            output[j] = toupper_char((unsigned int)input[i]);
            i++; j++;
        }
    }
}

static int match_dos_name(struct fat_directory_entry *entry, char *dos_name) {
    for(int k=0; k<8; k++) if(entry->name[k] != dos_name[k]) return 0;
    for(int k=0; k<3; k++) if(entry->ext[k] != dos_name[8+k]) return 0;
    return 1;
}

static void set_dos_name(struct fat_directory_entry *entry, char *dos_name) {
    for(int k=0; k<8; k++) entry->name[k] = dos_name[k];
    for(int k=0; k<3; k++) entry->ext[k] = dos_name[8+k];
}

void fat32_init() {
    uint32_t partition_lba_offset = 0;
    
    printf("[FAT32] Reading boot sector 0...\n");
    if (!disk_read_sector(0, fat_dma_buffer)) {
        printf(" [FAT32] Error: Could not read boot sector!\n");
        return;
    }

    if (fat_dma_buffer[510] != 0x55 || fat_dma_buffer[511] != 0xAA) {
        printf("[FAT32] Invalid Boot Signature!\n");
        return;
    }
    
    if (fat_dma_buffer[0] != 0xEB && fat_dma_buffer[0] != 0xE9) {
        printf("[FAT32] MBR detected. Locating FAT32 partition...\n");
        int found = 0;
        for (int p = 0; p < 4; p++) {
            uint8_t partition_type = fat_dma_buffer[0x1BE + (p * 16) + 4];
            if (partition_type == 0x0B || partition_type == 0x0C) {
                partition_lba_offset = *(uint32_t*)&fat_dma_buffer[0x1BE + (p * 16) + 8];
                printf("[FAT32] Found FAT32 at Partition %d (LBA %d)\n", p + 1, partition_lba_offset);
                found = 1;
                break;
            }
        }
        if (!found) {
            printf(" [FAT32] Error: No FAT32 partition found in MBR!\n");
            return;
        }
        if (!disk_read_sector(partition_lba_offset, fat_dma_buffer)) {
            printf("[FAT32] Error reading partition VBR!\n");
            return;
        }
    } else {
        printf("[FAT32] VBR (Superfloppy) detected.\n");
    }

    uint8_t* ptr = (uint8_t*)&bpb;
    for(int i=0; i<sizeof(struct fat32_bpb); i++) ptr[i] = fat_dma_buffer[i];

    if (bpb.sectors_per_cluster == 0) {
        printf("[FAT32] Error: Invalid sectors per cluster!\n");
        return;
    }

    char* type = (char*)bpb.fs_type;
    printf("[FAT32] FS Type detected: ");
    for(int i=0; i<5; i++) print_char(type[i]);
    printf("\n");

    fat_start_sector = partition_lba_offset + bpb.reserved_sectors;
    data_start_sector = partition_lba_offset + bpb.reserved_sectors + (bpb.num_fats * bpb.sectors_per_fat_32);
    
    printf("[FAT32] Ready. Root Cluster: ");
    printnumber(bpb.root_cluster);
    printf("\n");
}

uint32_t cluster_to_lba(uint32_t cluster) {
    if (bpb.sectors_per_cluster == 0) return 0;
    return data_start_sector + ((cluster - 2) * bpb.sectors_per_cluster);
}

uint32_t get_next_cluster(uint32_t current_cluster) {
    uint32_t fat_offset = current_cluster * 4;
    uint32_t fat_sector = fat_start_sector + (fat_offset / 512);
    uint32_t ent_offset = fat_offset % 512;
    
    if (!disk_read_sector(fat_sector, fat_dma_buffer)) return 0x0FFFFFFF;
    
    uint32_t val = *(uint32_t*)&fat_dma_buffer[ent_offset];
    return val & 0x0FFFFFFF;
}

void fat32_ls() {
    uint32_t current_cluster = bpb.root_cluster;

    while (current_cluster < 0x0FFFFFF8 && current_cluster != 0) {
        uint32_t lba = cluster_to_lba(current_cluster);
        
        for(int i=0; i<bpb.sectors_per_cluster; i++) {
            if (!disk_read_sector(lba + i, fat_dma_buffer)) return;
            
            struct fat_directory_entry* entry = (struct fat_directory_entry*) fat_dma_buffer;
            for(int j=0; j<16; j++) {
                if(entry[j].name[0] == 0x00) return;
                if(entry[j].name[0] == 0xE5) continue;
                if(entry[j].attributes == 0x0F) continue;
                
                char name[9];
                for(int k=0; k<8; k++) name[k] = entry[j].name[k];
                name[8] = 0;
                for(int k=7; k>=0 && name[k]==' '; k--) name[k] = 0;
                
                char ext[4];
                for(int k=0; k<3; k++) ext[k] = entry[j].ext[k];
                ext[3] = 0;
                
                printf(name);
                if(entry[j].attributes & 0x10) printf("/");
                else {
                    printf(".");
                    printf(ext);
                    printf("  ");
                    printf("%d", entry[j].file_size);
                    printf(" b");
                }
                printf("\n");
            }
        }
        current_cluster = get_next_cluster(current_cluster);
    }
}

int fat32_get_file_size(char* file_name){
    char dos_name[11];
    to_dos_file_name(file_name, dos_name);

    uint32_t dir_cluster = bpb.root_cluster;

    while (dir_cluster < 0x0FFFFFF8 && dir_cluster != 0) {
        uint32_t lba = cluster_to_lba(dir_cluster);
        for(int i=0; i<bpb.sectors_per_cluster; i++) {
            if (!disk_read_sector(lba + i, fat_dma_buffer)) return -1;
            
            struct fat_directory_entry* entry = (struct fat_directory_entry*) fat_dma_buffer;
            for(int j=0; j<16; j++) {
                if(entry[j].name[0] == 0x00) return -1;
                if(entry[j].name[0] == 0xE5 || entry[j].attributes == 0x0F) continue;
                
                if(match_dos_name(&entry[j], dos_name)) {
                    return entry[j].file_size;
                }
            }
        }
        dir_cluster = get_next_cluster(dir_cluster);
    }
    return -1;
}

int fat32_read_file(char* filename, uint8_t* out_buffer) {
    char dos_name[11];
    to_dos_file_name(filename, dos_name);
    
    uint32_t dir_cluster = bpb.root_cluster;
    
    while (dir_cluster < 0x0FFFFFF8 && dir_cluster != 0) {
        uint32_t lba = cluster_to_lba(dir_cluster);
        for(int i=0; i<bpb.sectors_per_cluster; i++) {
            if (!disk_read_sector(lba + i, fat_dma_buffer)) return -1;

            struct fat_directory_entry* entry = (struct fat_directory_entry*) fat_dma_buffer;
            for(int j=0; j<16; j++) {
                if(entry[j].name[0] == 0x00) return -1;
                if(entry[j].name[0] == 0xE5 || entry[j].attributes == 0x0F) continue;
                
                if(match_dos_name(&entry[j], dos_name)) {
                    uint32_t file_cluster = ((uint32_t)entry[j].first_cluster_high << 16) | entry[j].first_cluster_low;
                    uint32_t file_size = entry[j].file_size;
                    uint32_t bytes_read = 0;
                    
                    while(bytes_read < file_size && file_cluster < 0x0FFFFFF8 && file_cluster != 0) {
                        uint32_t file_lba = cluster_to_lba(file_cluster);
                        for(int s=0; s<bpb.sectors_per_cluster; s++) {
                            if (!disk_read_sector(file_lba + s, fat_dma_buffer)) return -1;
                            
                            int remaining = file_size - bytes_read;
                            int to_copy = (remaining < 512) ? remaining : 512;
                            
                            fast_memcpy(out_buffer + bytes_read, fat_dma_buffer, to_copy);
                            bytes_read += to_copy;
                            
                            if(bytes_read >= file_size) break; 
                        }
                        file_cluster = get_next_cluster(file_cluster);
                    }
                    return file_size;
                }
            }
        }
        dir_cluster = get_next_cluster(dir_cluster);
    }
    return -1;
}

void fat32_write_fat_entry(uint32_t cluster, uint32_t value) {
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fat_start_sector + (fat_offset / 512);
    uint32_t ent_offset = fat_offset % 512;
    
    if (!disk_read_sector(fat_sector, fat_dma_buffer)) return;
    
    uint32_t *entry = (uint32_t*)&fat_dma_buffer[ent_offset];
    uint32_t current_val = *entry;
    *entry = (current_val & 0xF0000000) | (value & 0x0FFFFFFF);
    
    if (!disk_write_sector(fat_sector, fat_dma_buffer)) return;
    if(bpb.num_fats > 1) {
        disk_write_sector(fat_sector + bpb.sectors_per_fat_32, fat_dma_buffer);
    }
}

uint32_t fat32_find_free_cluster() {
    uint32_t total_sectors = bpb.sectors_per_fat_32;
    
    for (uint32_t sec = 0; sec < total_sectors; sec++) {
        if (!disk_read_sector(fat_start_sector + sec, fat_dma_buffer)) return 0;
        uint32_t *entries = (uint32_t*)fat_dma_buffer;
        for (int i = 0; i < 128; i++) {
            if (sec == 0 && i < 2) continue; 
            if ((entries[i] & 0x0FFFFFFF) == 0) return sec * 128 + i;
        }
    }
    return 0;
}

int fat32_delete_file(char* filename) {
    if(isReadMode == 1) return -1;
    char dos_name[11];
    to_dos_file_name(filename, dos_name);
    
    uint32_t dir_cluster = bpb.root_cluster;
    
    while (dir_cluster < 0x0FFFFFF8 && dir_cluster != 0) {
        uint32_t lba = cluster_to_lba(dir_cluster);
        for(int i=0; i<bpb.sectors_per_cluster; i++) {
            if (!disk_read_sector(lba + i, fat_dma_buffer)) return -1;
            
            struct fat_directory_entry* entry = (struct fat_directory_entry*) fat_dma_buffer;
            for(int j=0; j<16; j++) {
                if(entry[j].name[0] == 0x00) return -1;
                if(entry[j].name[0] == 0xE5) continue;
                
                if(match_dos_name(&entry[j], dos_name)) {
                    uint32_t cluster = ((uint32_t)entry[j].first_cluster_high << 16) | entry[j].first_cluster_low;
                    
                    
                    while(cluster < 0x0FFFFFF8 && cluster != 0) {
                        uint32_t next = get_next_cluster(cluster);
                        fat32_write_fat_entry(cluster, 0); 
                        cluster = next;
                    }
                    
                    
                    disk_read_sector(lba + i, fat_dma_buffer);
                    entry = (struct fat_directory_entry*) fat_dma_buffer;
                    
                    entry[j].name[0] = 0xE5; 
                    disk_write_sector(lba + i, fat_dma_buffer);
                    return 1;
                }
            }
        }
        dir_cluster = get_next_cluster(dir_cluster);
    }
    return -1;
}

int fat32_write_file(char* filename, uint8_t* data, uint32_t size) {
    if(isReadMode == 1) return -1;
    char dos_name[11];
    to_dos_file_name(filename, dos_name);
    
    fat32_delete_file(filename);

    uint32_t cluster_bytes = bpb.sectors_per_cluster * 512;
    if (cluster_bytes == 0) return -3;
    uint32_t clusters_needed = (size + cluster_bytes - 1) / cluster_bytes;
    if (clusters_needed == 0) clusters_needed = 1;

    uint32_t first_cluster = 0;
    uint32_t prev_cluster = 0;

    for(uint32_t i = 0; i < clusters_needed; i++) {
        uint32_t free_cluster = fat32_find_free_cluster();
        if(free_cluster == 0) return -2;
        
        if(i == 0) first_cluster = free_cluster;
        else fat32_write_fat_entry(prev_cluster, free_cluster);
        
        fat32_write_fat_entry(free_cluster, 0x0FFFFFFF);
        prev_cluster = free_cluster;
    }

    uint32_t current_cluster = first_cluster;
    uint32_t written = 0;
    
    for(uint32_t i = 0; i < clusters_needed; i++) {
        uint32_t lba = cluster_to_lba(current_cluster);
        for(int s = 0; s < bpb.sectors_per_cluster; s++) {
            for(int z=0; z<512; z++) fat_dma_buffer[z] = 0; 

            if(written < size) {
                int rem = size - written;
                int to_copy = (rem < 512) ? rem : 512;
                fast_memcpy(fat_dma_buffer, data + written, to_copy);
                written += to_copy;
            }
            if (!disk_write_sector(lba + s, fat_dma_buffer)) return -3;
        }
        current_cluster = get_next_cluster(current_cluster);
    }

    uint32_t dir_cluster = bpb.root_cluster;
    
    while(1) {
        uint32_t lba = cluster_to_lba(dir_cluster);
        for(int i=0; i<bpb.sectors_per_cluster; i++) {
            if (!disk_read_sector(lba + i, fat_dma_buffer)) return -1;
            struct fat_directory_entry* entry = (struct fat_directory_entry*) fat_dma_buffer;
            
            for(int j=0; j<16; j++) {
                if(entry[j].name[0] == 0x00 || entry[j].name[0] == 0xE5) {
                    set_dos_name(&entry[j], dos_name);
                    entry[j].attributes = 0x20;
                    entry[j].first_cluster_high = (first_cluster >> 16) & 0xFFFF;
                    entry[j].first_cluster_low = first_cluster & 0xFFFF;
                    entry[j].file_size = size;
                    
                    disk_write_sector(lba + i, fat_dma_buffer);
                    return 1;
                }
            }
        }
        
        uint32_t next = get_next_cluster(dir_cluster);
        if(next >= 0x0FFFFFF8 || next == 0) {
            uint32_t new_dir_cluster = fat32_find_free_cluster();
            if (new_dir_cluster == 0) return -4; 
            
            fat32_write_fat_entry(dir_cluster, new_dir_cluster);
            fat32_write_fat_entry(new_dir_cluster, 0x0FFFFFFF);
            
            for(int z=0; z<512; z++) fat_dma_buffer[z] = 0;
            uint32_t new_lba = cluster_to_lba(new_dir_cluster);
            for(int s=0; s<bpb.sectors_per_cluster; s++) {
                disk_write_sector(new_lba + s, fat_dma_buffer);
            }
            dir_cluster = new_dir_cluster; 
        } else {
            dir_cluster = next;
        }
    }
    return -3;
}