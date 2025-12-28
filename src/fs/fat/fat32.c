#include "fs/fat/fat32.h"
#include "drivers/file/ATA/ATA.h"
#include "drivers/vga/vga.h"

struct fat32_bpb bpb;
uint32_t fat_start_sector;
uint32_t data_start_sector;

void to_dos_file_name(char* input, char* output) {
    for(int i=0; i<11; i++) output[i] = ' ';
    int i = 0, j = 0;
    while(input[i] != '\0' && input[i] != '.' && j < 8) {
        output[j++] = toupper_char(input[i++]);
    }
    if(input[i] == '.') {
        i++; j = 8;
        while(input[i] != '\0' && j < 11) {
            output[j++] = toupper_char(input[i++]);
        }
    }
}

void fat32_init() {
    uint8_t buffer[512];
    ata_read_sector(0, buffer, ATA_MASTER);
    
    uint8_t* ptr = (uint8_t*)&bpb;
    for(int i=0; i<sizeof(struct fat32_bpb); i++) ptr[i] = buffer[i];

    char* type = (char*)bpb.fs_type;
    print("FS Type detected: ");
    for(int i=0; i<5; i++) print_char(type[i]);
    print("\n");

    fat_start_sector = bpb.reserved_sectors;
    
    data_start_sector = bpb.reserved_sectors + (bpb.num_fats * bpb.sectors_per_fat_32);
    
    print("FAT32 Initialized. Root Cluster: ");
    printnumber(bpb.root_cluster);
    print("\n");
}

uint32_t cluster_to_lba(uint32_t cluster) {
    return data_start_sector + ((cluster - 2) * bpb.sectors_per_cluster);
}

uint32_t get_next_cluster(uint32_t current_cluster) {
    uint32_t fat_offset = current_cluster * 4;
    uint32_t fat_sector = fat_start_sector + (fat_offset / 512);
    uint32_t ent_offset = fat_offset % 512;
    
    uint8_t buffer[512];
    ata_read_sector(fat_sector, buffer, ATA_MASTER);
    
    uint32_t val = *(uint32_t*)&buffer[ent_offset];
    
    return val & 0x0FFFFFFF;
}

void fat32_ls() {
    uint32_t current_cluster = bpb.root_cluster;
    uint8_t buffer[512];

    while (current_cluster < 0x0FFFFFF8) {
        uint32_t lba = cluster_to_lba(current_cluster);
        
        for(int i=0; i<bpb.sectors_per_cluster; i++) {
            ata_read_sector(lba + i, buffer, ATA_MASTER);
            
            struct fat_directory_entry* entry = (struct fat_directory_entry*) buffer;
            
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
                
                print(name);
                if(entry[j].attributes & 0x10) print("/");
                else {
                    print(".");
                    print(ext);
                    print("  ");
                    printnumber(entry[j].file_size);
                    print(" b");
                }
                print("\n");
            }
        }
        
        current_cluster = get_next_cluster(current_cluster);
    }
}

int fat32_read_file(char* filename, uint8_t* out_buffer) {
    char dos_name[11];
    to_dos_file_name(filename, dos_name);
    
    uint32_t dir_cluster = bpb.root_cluster;
    uint8_t buffer[512];
    
    while (dir_cluster < 0x0FFFFFF8) {
        uint32_t lba = cluster_to_lba(dir_cluster);
        
        for(int i=0; i<bpb.sectors_per_cluster; i++) {
            ata_read_sector(lba + i, buffer, ATA_MASTER);
            struct fat_directory_entry* entry = (struct fat_directory_entry*) buffer;
            
            for(int j=0; j<16; j++) {
                if(entry[j].name[0] == 0x00) return -1;
                if(entry[j].name[0] == 0xE5 || entry[j].attributes == 0x0F) continue;
                
                int match = 1;
                for(int k=0; k<11; k++) {
                    if(entry[j].name[k] != dos_name[k]) { match = 0; break; }
                }
                
                if(match) {
                    uint32_t file_cluster = ((uint32_t)entry[j].first_cluster_high << 16) | entry[j].first_cluster_low;
                    uint32_t file_size = entry[j].file_size;
                    uint32_t bytes_read = 0;
                    
                    while(bytes_read < file_size && file_cluster < 0x0FFFFFF8) {
                        uint32_t file_lba = cluster_to_lba(file_cluster);
                        for(int s=0; s<bpb.sectors_per_cluster; s++) {
                            ata_read_sector(file_lba + s, out_buffer + bytes_read, ATA_MASTER);
                            bytes_read += 512;
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