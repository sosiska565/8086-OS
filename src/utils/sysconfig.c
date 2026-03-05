#include "utils/sysconfig.h"
#include "utils/utils.h"
#include "fs/vfs.h"
#include "global.h"
#include "memory/memory.h"
#include "drivers/vga/vga.h"
#include "drivers/video/graphics.h"

void sysconfig_reload(void) {
    int file_size = vfs_get_size("kernel.cfg");

    if(file_size <= 0){
        printf("[SysConfig] Warning: kernel.cfg not found! Using default values.\n");
        return;
    }

    uint8_t *file_buffer = (uint8_t*)kmalloc_a(file_size + 1);
    if(!file_buffer) return;

    for(int i = 0; i < file_size; i++) file_buffer[i] = 0;

    
    vfs_read("kernel.cfg", file_buffer);
    file_buffer[file_size] = '\0';

    Config *cfg = config_parse((char *)file_buffer);
    char *v;

    if((v = config_get_value(cfg, "is_read_only_mode"))) isReadMode = (strcmp(v, "true") == 0) ? 1 : 0;

    if((v = config_get_value(cfg, "taskbar_color"))) taskbar_color = (uint32_t)atoi(v, 16);
    if((v = config_get_value(cfg, "window_border_color"))) window_border_color = (uint32_t)atoi(v, 16);
    if((v = config_get_value(cfg, "window_active_border_color"))) window_active_border_color = (uint32_t)atoi(v, 16);
    
    if((v = config_get_value(cfg, "key_kill"))) key_kill = (uint8_t)atoi(v, 16);
    if((v = config_get_value(cfg, "key_focus"))) key_focus = (uint8_t)atoi(v, 16);
    if((v = config_get_value(cfg, "key_console"))) key_console = (uint8_t)atoi(v, 16);
    if((v = config_get_value(cfg, "key_layout"))) key_layout = (uint8_t)atoi(v, 16);
    if((v = config_get_value(cfg, "key_fullscreen"))) key_fullscreen = (uint8_t)atoi(v, 16);
    if((v = config_get_value(cfg, "key_ws_left"))) key_ws_left = (uint8_t)atoi(v, 16);
    if((v = config_get_value(cfg, "key_ws_right"))) key_ws_right = (uint8_t)atoi(v, 16);
    if((v = config_get_value(cfg, "key_resize_left"))) key_resize_left = (uint8_t)atoi(v, 16);
    if((v = config_get_value(cfg, "key_resize_right"))) key_resize_right = (uint8_t)atoi(v, 16);
    if((v = config_get_value(cfg, "key_resize_up"))) key_resize_up = (uint8_t)atoi(v, 16);
    if((v = config_get_value(cfg, "key_resize_down"))) key_resize_down = (uint8_t)atoi(v, 16);
    
    if((v = config_get_value(cfg, "wm_gaps"))) wm_gaps = atoi(v, 10);

    config_free(cfg);
    kfree_a(file_buffer);

    wm_refresh();
}

void sysconfig_init(void) {
    printf("[SysConfig] Loading system configuration...\n");
    sysconfig_reload();
}