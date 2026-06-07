#include "fs/fat/fat32.h"
#include "drivers/file/ATA/ATA.h"
#include "drivers/vga/vga.h"
#include "global.h"
#include "mm/memory.h" 
#include "utils/utils.h"

fat32_state_t fat_states[MAX_SYS_DRIVES];
static uint8_t fat_dma_buffer[512] __attribute__((aligned(4096)));

void to_dos_file_name(char* input, char* output) {
    for(int k=0; k<11; k++) output[k] = ' ';
    int i = 0, j = 0;
    while(input[i] != '\0' && input[i] != '.' && j < 8) { output[j] = toupper_char((unsigned int)input[i]); i++; j++; }
    if(input[i] == '.') { i++; j = 8; while(input[i] != '\0' && j < 11) { output[j] = toupper_char((unsigned int)input[i]); i++; j++; } }
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


static void extract_lfn_chars(struct fat_lfn_entry *lfn, char *lfn_buf) {
    int idx = ((lfn->order & 0x1F) - 1) * 13;
    if (idx < 0 || idx >= 256 - 13) return; 
    
    char temp[13];
    int j = 0;
    
    for (int i = 0; i < 5; i++) temp[j++] = lfn->name1[i * 2];
    for (int i = 0; i < 6; i++) temp[j++] = lfn->name2[i * 2];
    for (int i = 0; i < 2; i++) temp[j++] = lfn->name3[i * 2];
    
    for (int i = 0; i < 13; i++) {
        if (temp[i] == 0x00 || temp[i] == (char)0xFF) {
            lfn_buf[idx + i] = '\0';
            break;
        }
        lfn_buf[idx + i] = temp[i];
    }
    
    
    if (lfn->order & 0x40) {
        lfn_buf[idx + 13] = '\0';
    }
}


static int fat32_streq(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        char c1 = (*s1 >= 'A' && *s1 <= 'Z') ? *s1 + 32 : *s1;
        char c2 = (*s2 >= 'A' && *s2 <= 'Z') ? *s2 + 32 : *s2;
        if (c1 != c2) return 0;
        s1++; s2++;
    }
    return *s1 == *s2;
}

int fat32_mount(int drive_id) {
    if (drive_id < 0 || drive_id >= sys_drive_count) return -1;
    
    fat32_state_t *fs = &fat_states[drive_id];
    uint32_t partition_lba_offset = 0;
    
    if (!disk_read_sector(drive_id, 0, fat_dma_buffer)) return -1;

    if (fat_dma_buffer[0] != 0xEB && fat_dma_buffer[0] != 0xE9) {
        for (int p = 0; p < 4; p++) {
            uint8_t pt = fat_dma_buffer[0x1BE + (p * 16) + 4];
            if (pt == 0x0B || pt == 0x0C) { 
                partition_lba_offset = *(uint32_t*)&fat_dma_buffer[0x1BE + (p * 16) + 8]; 
                break; 
            }
        }
        if (!disk_read_sector(drive_id, partition_lba_offset, fat_dma_buffer)) return -1;
    }

    uint8_t* ptr = (uint8_t*)&fs->bpb;
    for(int i=0; i<sizeof(struct fat32_bpb); i++) ptr[i] = fat_dma_buffer[i];

    if (fs->bpb.sectors_per_cluster == 0 || fs->bpb.bytes_per_sector == 0) return -1;

    fs->fat_start_sector = partition_lba_offset + fs->bpb.reserved_sectors;
    fs->data_start_sector = partition_lba_offset + fs->bpb.reserved_sectors + (fs->bpb.num_fats * fs->bpb.sectors_per_fat_32);
    fs->last_free_fat_sector = 0; 
    fs->is_mounted = 1;

    return 0; 
}

void fat32_write_fat_entry(int drive_id, uint32_t cluster, uint32_t value) {
    fat32_state_t *fs = &fat_states[drive_id];
    if (cluster < 2) return;
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fs->fat_start_sector + (fat_offset / 512);
    uint32_t ent_offset = fat_offset % 512;
    disk_read_sector(drive_id, fat_sector, fat_dma_buffer);
    uint32_t *entry = (uint32_t*)&fat_dma_buffer[ent_offset];
    *entry = (*entry & 0xF0000000) | (value & 0x0FFFFFFF);
    disk_write_sector(drive_id, fat_sector, fat_dma_buffer);
    if(fs->bpb.num_fats > 1) disk_write_sector(drive_id, fat_sector + fs->bpb.sectors_per_fat_32, fat_dma_buffer);
}

uint32_t cluster_to_lba(int drive_id, uint32_t cluster) {
    fat32_state_t *fs = &fat_states[drive_id];
    if (cluster < 2) return 0; 
    return fs->data_start_sector + ((cluster - 2) * fs->bpb.sectors_per_cluster);
}

uint32_t get_next_cluster(int drive_id, uint32_t current_cluster) {
    fat32_state_t *fs = &fat_states[drive_id];
    if (current_cluster < 2) return 0x0FFFFFFF;
    uint32_t fat_offset = current_cluster * 4;
    uint32_t fat_sector = fs->fat_start_sector + (fat_offset / 512);
    uint32_t ent_offset = fat_offset % 512;
    if (!disk_read_sector(drive_id, fat_sector, fat_dma_buffer)) return 0x0FFFFFFF;
    uint32_t val = *(uint32_t*)&fat_dma_buffer[ent_offset];
    return val & 0x0FFFFFFF;
}

uint32_t fat32_find_free_cluster(int drive_id) {
    fat32_state_t *fs = &fat_states[drive_id];
    for (uint32_t sec = fs->last_free_fat_sector; sec < fs->bpb.sectors_per_fat_32; sec++) {
        if (!disk_read_sector(drive_id, fs->fat_start_sector + sec, fat_dma_buffer)) return 0;
        uint32_t *entries = (uint32_t*)fat_dma_buffer;
        for (int i = 0; i < 128; i++) {
            if (sec == 0 && i < 2) continue; 
            if ((entries[i] & 0x0FFFFFFF) == 0) {
                fs->last_free_fat_sector = sec; 
                return sec * 128 + i;
            }
        }
    }
    return 0; 
}

uint32_t fat32_get_cluster_for_path(int drive_id, char* path, uint8_t* out_type, uint32_t* out_size) {
    fat32_state_t *fs = &fat_states[drive_id];
    if (strcmp(path, "/") == 0 || path[0] == '\0') {
        if (out_type) *out_type = VFS_ATTR_DIR;
        if (out_size) *out_size = 0;
        return fs->bpb.root_cluster;
    }

    uint32_t current_cluster = fs->bpb.root_cluster;
    char temp_path[256]; strcpy(temp_path, path);
    char* token = temp_path;
    if (*token == '/') token++; 

    while (*token) {
        char* next_slash = token;
        while (*next_slash && *next_slash != '/') next_slash++;
        if (*next_slash == '/') { *next_slash = '\0'; next_slash++; }

        if (*token == '\0') { token = next_slash; continue; } 

        char dos_name[11];
        to_dos_file_name(token, dos_name);
        int found = 0, end_of_dir = 0;

        uint32_t search_cluster = current_cluster;
        int safety_limit = 1000;
        
        char lfn_buf[256];
        memset(lfn_buf, 0, 256);

        while (search_cluster >= 2 && search_cluster < 0x0FFFFFF8 && !end_of_dir && safety_limit-- > 0) {
            uint32_t lba = cluster_to_lba(drive_id, search_cluster);
            if (lba == 0) break;

            for (int i = 0; i < fs->bpb.sectors_per_cluster; i++) {
                if (!disk_read_sector(drive_id, lba + i, fat_dma_buffer)) { end_of_dir = 1; break; }
                struct fat_directory_entry* entry = (struct fat_directory_entry*)fat_dma_buffer;
                
                for (int j = 0; j < 16; j++) {
                    if (entry[j].name[0] == 0x00) { end_of_dir = 1; break; } 
                    if (entry[j].name[0] == 0xE5) { memset(lfn_buf, 0, 256); continue; }
                    
                    if (entry[j].attributes == 0x0F) {
                        extract_lfn_chars((struct fat_lfn_entry*)&entry[j], lfn_buf);
                        continue;
                    }
                    
                    if (entry[j].attributes & 0x08) { memset(lfn_buf, 0, 256); continue; } 

                    int is_match = 0;
                    if (lfn_buf[0] != '\0' && fat32_streq(lfn_buf, token)) {
                        is_match = 1;
                    } else if (match_dos_name(&entry[j], dos_name)) {
                        is_match = 1;
                    }

                    if (is_match) {
                        current_cluster = ((uint32_t)entry[j].first_cluster_high << 16) | entry[j].first_cluster_low;
                        if (current_cluster == 0) current_cluster = fs->bpb.root_cluster; 
                        if (out_type) *out_type = (entry[j].attributes & 0x10) ? VFS_ATTR_DIR : VFS_ATTR_FILE;
                        if (out_size) *out_size = entry[j].file_size;
                        found = 1; break;
                    }
                    memset(lfn_buf, 0, 256);
                }
                if (found || end_of_dir) break;
            }
            if (found || end_of_dir) break;
            
            uint32_t next = get_next_cluster(drive_id, search_cluster);
            if (next == search_cluster || next < 2) break;
            search_cluster = next;
        }

        if (!found) return 0xFFFFFFFF; 
        token = next_slash;
    }
    return current_cluster;
}

int fat32_readdir(int drive_id, char* path, int index, vfs_dirent_t* out_dirent) {
    fat32_state_t *fs = &fat_states[drive_id];
    uint8_t type;
    uint32_t dir_cluster = fat32_get_cluster_for_path(drive_id, path, &type, NULL);
    if (dir_cluster == 0xFFFFFFFF || type != VFS_ATTR_DIR) return 0; 

    int current_index = 0;
    int safety_limit = 1000;
    char lfn_buf[256];
    memset(lfn_buf, 0, 256);
    
    while (dir_cluster >= 2 && dir_cluster < 0x0FFFFFF8 && safety_limit-- > 0) {
        uint32_t lba = cluster_to_lba(drive_id, dir_cluster);
        if (lba == 0) break;

        for(int i = 0; i < fs->bpb.sectors_per_cluster; i++) {
            if (!disk_read_sector(drive_id, lba + i, fat_dma_buffer)) return 0;
            struct fat_directory_entry* entry = (struct fat_directory_entry*) fat_dma_buffer;
            
            for(int j = 0; j < 16; j++) {
                if(entry[j].name[0] == 0x00) return 0; 
                if(entry[j].name[0] == 0xE5) { memset(lfn_buf, 0, 256); continue; } 
                
                if (entry[j].attributes == 0x0F) {
                    extract_lfn_chars((struct fat_lfn_entry*)&entry[j], lfn_buf);
                    continue;
                }
                
                if (entry[j].attributes & 0x08) { memset(lfn_buf, 0, 256); continue; }
                if (entry[j].name[0] == '.') { memset(lfn_buf, 0, 256); continue; } 
                
                if (current_index == index) {
                    if (lfn_buf[0] != '\0') {
                        strcpy(out_dirent->name, lfn_buf);
                    } else {
                        int pos = 0;
                        for(int k=0; k<8 && entry[j].name[k] != ' '; k++) out_dirent->name[pos++] = entry[j].name[k];
                        if (entry[j].ext[0] != ' ') {
                            out_dirent->name[pos++] = '.';
                            for(int k=0; k<3 && entry[j].ext[k] != ' '; k++) out_dirent->name[pos++] = entry[j].ext[k];
                        }
                        out_dirent->name[pos] = '\0';
                    }
                    
                    out_dirent->size = entry[j].file_size;
                    out_dirent->type = (entry[j].attributes & 0x10) ? VFS_ATTR_DIR : VFS_ATTR_FILE;
                    return 1;
                }
                current_index++;
                memset(lfn_buf, 0, 256);
            }
        }
        uint32_t next = get_next_cluster(drive_id, dir_cluster);
        if (next == dir_cluster || next < 2) break;
        dir_cluster = next;
    }
    return 0;
}

int fat32_mkdir(int drive_id, char* path) {
    if (isReadMode == 1) return -1;
    fat32_state_t *fs = &fat_states[drive_id];
    
    char parent_path[256]; char new_dir_name[64];
    int last_slash = -1; int len = strlen(path);
    for(int i=len-1; i>=0; i--) { if(path[i] == '/') { last_slash = i; break; } }
    if (last_slash == -1) return -1;
    if (last_slash == 0) { strcpy(parent_path, "/"); } 
    else { for(int i=0; i<last_slash; i++) parent_path[i] = path[i]; parent_path[last_slash] = '\0'; }
    strcpy(new_dir_name, path + last_slash + 1);
    
    uint8_t parent_type;
    uint32_t parent_cluster = fat32_get_cluster_for_path(drive_id, parent_path, &parent_type, NULL);
    if (parent_cluster == 0xFFFFFFFF || parent_type != VFS_ATTR_DIR) return -1;
    
    char dos_name[11]; to_dos_file_name(new_dir_name, dos_name);
    uint32_t new_cluster = fat32_find_free_cluster(drive_id);
    if (new_cluster == 0) return -2;
    fat32_write_fat_entry(drive_id, new_cluster, 0x0FFFFFFF);
    
    uint32_t lba = cluster_to_lba(drive_id, new_cluster);
    for(int s=0; s<fs->bpb.sectors_per_cluster; s++) {
        fast_memset(fat_dma_buffer, 0, 512 / 4); disk_write_sector(drive_id, lba + s, fat_dma_buffer);
    }
    
    disk_read_sector(drive_id, lba, fat_dma_buffer);
    struct fat_directory_entry* entry = (struct fat_directory_entry*)fat_dma_buffer;
    fast_memset(&entry[0], 0, sizeof(struct fat_directory_entry) / 4);
    for(int i=0; i<11; i++) entry[0].name[i] = ' ';
    entry[0].name[0] = '.'; entry[0].attributes = 0x10; 
    entry[0].first_cluster_high = (new_cluster >> 16) & 0xFFFF; entry[0].first_cluster_low = new_cluster & 0xFFFF;
    
    fast_memset(&entry[1], 0, sizeof(struct fat_directory_entry) / 4);
    for(int i=0; i<11; i++) entry[1].name[i] = ' ';
    entry[1].name[0] = '.'; entry[1].name[1] = '.'; entry[1].attributes = 0x10;
    uint32_t parent_link = (parent_cluster == fs->bpb.root_cluster) ? 0 : parent_cluster;
    entry[1].first_cluster_high = (parent_link >> 16) & 0xFFFF; entry[1].first_cluster_low = parent_link & 0xFFFF;
    disk_write_sector(drive_id, lba, fat_dma_buffer);
    
    uint32_t current_cluster = parent_cluster;
    int safety_limit = 1000;
    while(current_cluster >= 2 && current_cluster < 0x0FFFFFF8 && safety_limit-- > 0) {
        uint32_t plba = cluster_to_lba(drive_id, current_cluster);
        for(int i=0; i<fs->bpb.sectors_per_cluster; i++) {
            disk_read_sector(drive_id, plba + i, fat_dma_buffer);
            entry = (struct fat_directory_entry*)fat_dma_buffer;
            for(int j=0; j<16; j++) {
                if (entry[j].name[0] == 0x00 || entry[j].name[0] == 0xE5) {
                    uint8_t was_end = (entry[j].name[0] == 0x00); 
                    fast_memset(&entry[j], 0, sizeof(struct fat_directory_entry) / 4);
                    set_dos_name(&entry[j], dos_name);
                    entry[j].attributes = 0x10;
                    entry[j].first_cluster_high = (new_cluster >> 16) & 0xFFFF;
                    entry[j].first_cluster_low = new_cluster & 0xFFFF;
                    if (was_end && j + 1 < 16) entry[j+1].name[0] = 0x00;
                    disk_write_sector(drive_id, plba + i, fat_dma_buffer);
                    if (was_end && j == 15 && i + 1 < fs->bpb.sectors_per_cluster) {
                        disk_read_sector(drive_id, plba + i + 1, fat_dma_buffer);
                        fat_dma_buffer[0] = 0x00;
                        disk_write_sector(drive_id, plba + i + 1, fat_dma_buffer);
                    }
                    return 1;
                }
            }
        }
        uint32_t next = get_next_cluster(drive_id, current_cluster);
        if (next == current_cluster || next < 2) break;
        current_cluster = next;
    }
    return -1;
}

int fat32_get_file_size(int drive_id, char* filename) {
    uint8_t type; uint32_t size;
    uint32_t cluster = fat32_get_cluster_for_path(drive_id, filename, &type, &size);
    if (cluster == 0xFFFFFFFF || type != VFS_ATTR_FILE) return -1;
    return size;
}

int fat32_read_file(int drive_id, char* filename, uint8_t* out_buffer) {
    fat32_state_t *fs = &fat_states[drive_id];
    uint8_t type; uint32_t size;
    uint32_t cluster = fat32_get_cluster_for_path(drive_id, filename, &type, &size);
    if (cluster == 0xFFFFFFFF || type != VFS_ATTR_FILE) return -1;
    
    uint32_t bytes_read = 0;
    int safety_limit = 10000;
    while(bytes_read < size && cluster >= 2 && cluster < 0x0FFFFFF8 && safety_limit-- > 0) {
        uint32_t file_lba = cluster_to_lba(drive_id, cluster);
        if (file_lba == 0) break;
        for(int s=0; s<fs->bpb.sectors_per_cluster; s++) {
            if (!disk_read_sector(drive_id, file_lba + s, fat_dma_buffer)) return -1;
            int remaining = size - bytes_read;
            int to_copy = (remaining < 512) ? remaining : 512;
            fast_memcpy(out_buffer + bytes_read, fat_dma_buffer, to_copy);
            bytes_read += to_copy;
            if(bytes_read >= size) break; 
        }
        uint32_t next = get_next_cluster(drive_id, cluster);
        if (next == cluster || next < 2) break;
        cluster = next;
    }
    return size;
}

int fat32_delete_file(int drive_id, char* filename) {
    if (isReadMode == 1) return -1;
    fat32_state_t *fs = &fat_states[drive_id];

    char parent_path[256]; char target_file_name[64];
    int last_slash = -1; int len = strlen(filename);
    for(int i = len - 1; i >= 0; i--) if(filename[i] == '/') { last_slash = i; break; }
    if (last_slash == -1) return -1;
    if (last_slash == 0) { strcpy(parent_path, "/"); } 
    else { for(int i = 0; i < last_slash; i++) parent_path[i] = filename[i]; parent_path[last_slash] = '\0'; }
    strcpy(target_file_name, filename + last_slash + 1);

    uint8_t parent_type;
    uint32_t parent_cluster = fat32_get_cluster_for_path(drive_id, parent_path, &parent_type, NULL);
    if (parent_cluster == 0xFFFFFFFF || parent_type != VFS_ATTR_DIR) return -1;

    char dos_name[11]; to_dos_file_name(target_file_name, dos_name);
    char lfn_buf[256]; memset(lfn_buf, 0, 256);

    uint32_t current_cluster = parent_cluster;
    int safety_limit = 1000;
    while(current_cluster >= 2 && current_cluster < 0x0FFFFFF8 && safety_limit-- > 0) {
        uint32_t plba = cluster_to_lba(drive_id, current_cluster);
        for(int i = 0; i < fs->bpb.sectors_per_cluster; i++) {
            disk_read_sector(drive_id, plba + i, fat_dma_buffer);
            struct fat_directory_entry* entry = (struct fat_directory_entry*)fat_dma_buffer;
            for(int j = 0; j < 16; j++) {
                if (entry[j].name[0] == 0x00) break;
                if (entry[j].name[0] == 0xE5) { memset(lfn_buf, 0, 256); continue; }
                if (entry[j].attributes == 0x0F) {
                    extract_lfn_chars((struct fat_lfn_entry*)&entry[j], lfn_buf);
                    continue;
                }
                
                int is_match = 0;
                if (lfn_buf[0] != '\0' && fat32_streq(lfn_buf, target_file_name)) is_match = 1;
                else if (match_dos_name(&entry[j], dos_name)) is_match = 1;

                if (is_match) {
                    uint32_t target_cluster = ((uint32_t)entry[j].first_cluster_high << 16) | entry[j].first_cluster_low;
                    entry[j].name[0] = 0xE5;
                    disk_write_sector(drive_id, plba + i, fat_dma_buffer);
                    uint32_t c = target_cluster;
                    while(c >= 2 && c < 0x0FFFFFF8) {
                        uint32_t next = get_next_cluster(drive_id, c);
                        fat32_write_fat_entry(drive_id, c, 0); 
                        uint32_t fat_sector = (c * 4) / 512;
                        if (fat_sector < fs->last_free_fat_sector) fs->last_free_fat_sector = fat_sector;
                        c = next;
                    }
                    return 1;
                }
                memset(lfn_buf, 0, 256);
            }
        }
        uint32_t next = get_next_cluster(drive_id, current_cluster);
        if (next == current_cluster || next < 2) break;
        current_cluster = next;
    }
    return -1;
}

int fat32_write_file(int drive_id, char* filename, uint8_t* data, uint32_t size) {
    if (isReadMode == 1) return -1;
    fat32_state_t *fs = &fat_states[drive_id];

    fat32_delete_file(drive_id, filename);

    char parent_path[256]; char new_file_name[64];
    int last_slash = -1; int len = strlen(filename);
    for(int i = len - 1; i >= 0; i--) if(filename[i] == '/') { last_slash = i; break; }
    if (last_slash == -1) return -1;
    if (last_slash == 0) { strcpy(parent_path, "/"); } 
    else { for(int i = 0; i < last_slash; i++) parent_path[i] = filename[i]; parent_path[last_slash] = '\0'; }
    strcpy(new_file_name, filename + last_slash + 1);

    uint8_t parent_type;
    uint32_t parent_cluster = fat32_get_cluster_for_path(drive_id, parent_path, &parent_type, NULL);
    if (parent_cluster == 0xFFFFFFFF || parent_type != VFS_ATTR_DIR) return -1;

    char dos_name[11]; to_dos_file_name(new_file_name, dos_name);

    uint32_t clusters_needed = (size + (fs->bpb.sectors_per_cluster * 512) - 1) / (fs->bpb.sectors_per_cluster * 512);
    if (size == 0) clusters_needed = 1;

    uint32_t first_cluster = 0; uint32_t prev_cluster = 0;

    for (uint32_t i = 0; i < clusters_needed; i++) {
        uint32_t new_cluster = fat32_find_free_cluster(drive_id);
        if (new_cluster == 0) return -2;

        fat32_write_fat_entry(drive_id, new_cluster, 0x0FFFFFFF); 
        if (prev_cluster != 0) fat32_write_fat_entry(drive_id, prev_cluster, new_cluster); 
        else first_cluster = new_cluster;

        uint32_t lba = cluster_to_lba(drive_id, new_cluster);
        uint32_t bytes_to_write = (size > (i * fs->bpb.sectors_per_cluster * 512)) ? (size - (i * fs->bpb.sectors_per_cluster * 512)) : 0;

        for (int s = 0; s < fs->bpb.sectors_per_cluster; s++) {
            fast_memset(fat_dma_buffer, 0, 512 / 4);
            if (bytes_to_write > 0) {
                uint32_t chunk = (bytes_to_write > 512) ? 512 : bytes_to_write;
                fast_memcpy(fat_dma_buffer, data + (i * fs->bpb.sectors_per_cluster * 512) + (s * 512), chunk);
                bytes_to_write -= chunk;
            }
            disk_write_sector(drive_id, lba + s, fat_dma_buffer);
        }
        prev_cluster = new_cluster;
    }

    uint32_t current_cluster = parent_cluster;
    int safety_limit = 1000;
    while(current_cluster >= 2 && current_cluster < 0x0FFFFFF8 && safety_limit-- > 0) {
        uint32_t plba = cluster_to_lba(drive_id, current_cluster);
        for(int i = 0; i < fs->bpb.sectors_per_cluster; i++) {
            disk_read_sector(drive_id, plba + i, fat_dma_buffer);
            struct fat_directory_entry* entry = (struct fat_directory_entry*)fat_dma_buffer;
            for(int j = 0; j < 16; j++) {
                if (entry[j].name[0] == 0x00 || entry[j].name[0] == 0xE5) {
                    uint8_t was_end = (entry[j].name[0] == 0x00); 
                    fast_memset(&entry[j], 0, sizeof(struct fat_directory_entry) / 4);
                    set_dos_name(&entry[j], dos_name);
                    entry[j].attributes = 0x20; 
                    entry[j].first_cluster_high = (first_cluster >> 16) & 0xFFFF;
                    entry[j].first_cluster_low = first_cluster & 0xFFFF;
                    entry[j].file_size = size;
                    if (was_end && j + 1 < 16) entry[j+1].name[0] = 0x00;
                    disk_write_sector(drive_id, plba + i, fat_dma_buffer);
                    if (was_end && j == 15 && i + 1 < fs->bpb.sectors_per_cluster) {
                        disk_read_sector(drive_id, plba + i + 1, fat_dma_buffer);
                        fat_dma_buffer[0] = 0x00;
                        disk_write_sector(drive_id, plba + i + 1, fat_dma_buffer);
                    }
                    return 1;
                }
            }
        }
        uint32_t next = get_next_cluster(drive_id, current_cluster);
        if (next == current_cluster || next < 2) break;
        current_cluster = next;
    }
    return -1;
}