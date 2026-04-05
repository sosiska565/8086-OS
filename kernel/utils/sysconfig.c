#include "utils/sysconfig.h"
#include "utils/utils.h"
#include "fs/vfs.h"
#include "global.h"
#include "mm/memory.h"
#include "drivers/vga/vga.h"

Config *global_cfg = 0;

void sysconfig_reload(void) {
    int file_size = vfs_get_size("/kernel.cfg");
    
    
    if(file_size <= 0) {
        char* def = "PROMPT_USER_COLOR=11\nPROMPT_HOST_COLOR=10\nPROMPT_PATH_COLOR=9\nPATH=/path\n";
        vfs_write("/kernel.cfg", (uint8_t*)def, strlen(def));
        file_size = strlen(def);
    }
    
    uint8_t *file_buffer = (uint8_t*)kmalloc_a(file_size + 1);
    vfs_read("/kernel.cfg", file_buffer);
    file_buffer[file_size] = '\0';
    
    if (global_cfg) config_free(global_cfg);
    global_cfg = config_parse((char *)file_buffer);

    kfree_a(file_buffer);
}

void sysconfig_init(void) { sysconfig_reload(); }