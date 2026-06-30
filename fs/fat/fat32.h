#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>
#include "fs/vfs.h"
#include "drivers/file/ATA/ATA.h"

struct fat32_bpb {
    uint8_t  jmp_boot[3];
    uint8_t  oem_name[8];
    uint16_t bytes_per_sector;       
    uint8_t  sectors_per_cluster;    
    uint16_t reserved_sectors;       
    uint8_t  num_fats;               
    uint16_t root_dir_entries;
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t sectors_per_fat_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;

    uint32_t sectors_per_fat_32;
    uint16_t ext_flags;
    uint16_t fs_ver;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t bk_boot_sec;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    uint8_t  volume_label[11];
    uint8_t  fs_type[8];
} __attribute__((packed));

struct fat_directory_entry {
    uint8_t  name[8];
    uint8_t  ext[3];
    uint8_t  attributes;
    uint8_t  reserved;
    uint8_t  c_time_tenth;
    uint16_t c_time;
    uint16_t c_date;
    uint16_t a_date;
    uint16_t first_cluster_high;
    uint16_t w_time;
    uint16_t w_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} __attribute__((packed));


struct fat_lfn_entry {
    uint8_t  order;
    uint8_t  name1[10];
    uint8_t  attributes; 
    uint8_t  type;       
    uint8_t  checksum;
    uint8_t  name2[12];
    uint16_t zero;
    uint8_t  name3[4];
} __attribute__((packed));

typedef struct {
    struct fat32_bpb bpb;
    uint32_t fat_start_sector;
    uint32_t data_start_sector;
    uint32_t last_free_fat_sector;
    int is_mounted;
} fat32_state_t;

extern fat32_state_t fat_states[MAX_SYS_DRIVES];

int fat32_mount(int drive_id);
int fat32_read_file(int drive_id, char* filename, uint8_t* out_buffer);
int fat32_get_file_size(int drive_id, char* file_name);
int fat32_write_file(int drive_id, char* filename, uint8_t* buffer, uint32_t size);
int fat32_delete_file(int drive_id, char* filename);
int fat32_readdir(int drive_id, char* path, int index, vfs_dirent_t* out_dirent);
int fat32_mkdir(int drive_id, char* path);

uint32_t fat32_get_cluster_for_path(int drive_id, char* path, uint8_t* out_type, uint32_t* out_size);

#endif