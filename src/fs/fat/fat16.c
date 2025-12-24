#include "fs/fat/fat16.h"
#include "drivers/file/ATA/ATA.h"
#include "drivers/vga/vga.h"

struct fat_bpb bpb;
uint8_t bpb_buffer[512];
uint32_t root_dir_start_sector;

void to_dos_file_name(char* input, char* output, int output_len) {
    for(int i=0; i<output_len; i++) output[i] = ' ';
    
    int i = 0;
    int j = 0;
    
    while(input[i] != '\0' && input[i] != '.' && j < 8) {
        output[j] = toupper_char(input[i]);
        i++;
        j++;
    }
    
    if(input[i] == '.') {
        i++;
        j = 8;
        while(input[i] != '\0' && j < 11) {
            output[j] = toupper_char(input[i]);
            i++;
            j++;
        }
    }
}

void fat16_init(void){
    uint8_t buffer[512];
    ata_read_sector(0, buffer);
    
    uint8_t* dest = (uint8_t*)&bpb;
    for(int i=0; i<sizeof(struct fat_bpb); i++) {
        dest[i] = buffer[i];
    }

    root_dir_start_sector = bpb.reserved_sectors + (bpb.num_fats * bpb.sectors_per_fat);

    print_info("FAT16", "Initialized. Root Dir starts at LBA: ", VGA_COLOR_GREEN, VGA_COLOR_LIGHT_GREY);
    printnumber(root_dir_start_sector);
    print("\n");
}

void fat16_ls(void) {
    uint8_t buffer[512];
    
    uint32_t root_dir_size = (bpb.root_dir_entries * 32) / 512;
    
    for(int i = 0; i < root_dir_size; i++) {
        ata_read_sector(root_dir_start_sector + i, buffer);
        
        struct fat_directory_entry* entry = (struct fat_directory_entry*) buffer;
        
        for(int j = 0; j < 16; j++) {
            
            if (entry[j].name[0] == 0x00) return;
            
            if (entry[j].name[0] == 0xE5) continue;
            
            if (entry[j].attributes == 0x0F) continue;
            if (entry[j].attributes & 0x08) continue; 
            
            char name[9];
            char ext[4];
            
            for(int k=0; k<8; k++) name[k] = entry[j].name[k];
            name[8] = '\0';
            
            for(int k=7; k>=0; k--) {
                if(name[k] == ' ') name[k] = '\0';
                else break;
            }

            for(int k=0; k<3; k++) ext[k] = entry[j].ext[k];
            ext[3] = '\0';

            print(name);
            if(entry[j].attributes & 0x10) {
                print("/");
            } else {
                print(".");
                print(ext);
            }
            
            print("     Size: ");
            printnumber(entry[j].file_size);
            print(" bytes");
            
            print("\n");
        }
    }
}

int fat16_read_file(char* filename, uint8_t* buffer) {
    char dos_name[11];
    to_dos_file_name(filename, dos_name, 11);
    
    uint32_t root_dir_size = (bpb.root_dir_entries * 32) / 512;
    uint32_t data_start_sector = root_dir_start_sector + root_dir_size;
    
    uint8_t sector_buffer[512];
    
    for(int i = 0; i < root_dir_size; i++) {
        ata_read_sector(root_dir_start_sector + i, sector_buffer);
        struct fat_directory_entry* entry = (struct fat_directory_entry*) sector_buffer;
        
        for(int j = 0; j < 16; j++) {
            if (entry[j].name[0] == 0x00) return -1;
            if (entry[j].name[0] == 0xE5) continue;
            if (entry[j].attributes == 0x0F) continue;
            
            int match = 1;
            for(int k=0; k<11; k++) {
                if (entry[j].name[k] != dos_name[k]) {
                    match = 0;
                    break;
                }
            }
            
            if (match) {
                uint16_t cluster = entry[j].first_cluster_low;
                
                uint32_t file_sector = data_start_sector + ((cluster - 2) * bpb.sectors_per_cluster);
                
                ata_read_sector(file_sector, buffer);
                return entry[j].file_size;
            }
        }
    }
    
    return 0;
}