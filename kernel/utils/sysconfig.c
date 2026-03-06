#include "utils/sysconfig.h"
#include "utils/utils.h"
#include "fs/vfs.h"
#include "global.h"
#include "mm/memory.h"
#include "drivers/vga/vga.h"
#include "drivers/video/graphics.h"

char wallpaper_path[128] = "";
uint32_t *wallpaper_buf = 0;
int font_scale = 1;

#pragma pack(push, 1)
typedef struct { uint16_t type; uint32_t size; uint16_t res1, res2; uint32_t offset; } BMPHeader;
typedef struct { uint32_t size; int32_t w, h; uint16_t planes, bpp; uint32_t comp, img_size; int32_t x_ppm, y_ppm; uint32_t colors, imp_colors; } BMPInfo;
#pragma pack(pop)

void load_wallpaper(char* path) {
    if(wallpaper_buf) { kfree_a(wallpaper_buf); wallpaper_buf = 0; }
    int sz = vfs_get_size(path);
    if(sz <= 0) {
        printf("[WM] Wallpaper '%s' not found or empty.\n", path);
        return;
    }
    
    uint8_t *file = kmalloc_a(sz);
    vfs_read(path, file);
    
    BMPHeader *h = (BMPHeader*)file;
    BMPInfo *info = (BMPInfo*)(file + sizeof(BMPHeader));
    
    if (h->type != 0x4D42 || (info->bpp != 24 && info->bpp != 32)) { 
        printf("[WM] Wallpaper format error! Must be 24/32 bpp BMP.\n");
        kfree_a(file); 
        return; 
    }
    
    if (info->w <= 0 || info->w > 4096 || info->h == 0) {
        printf("[WM] Wallpaper dimensions error!\n");
        kfree_a(file);
        return;
    }

    extern int screen_width, screen_height, screen_bpp, screen_pitch;
    int bpp_bytes = screen_bpp / 8;
    
    
    wallpaper_buf = (uint32_t*)kmalloc_a(screen_pitch * screen_height);
    
    uint8_t *pixels = file + h->offset;
    int bmp_w = info->w, bmp_h = info->h;
    if(bmp_h < 0) bmp_h = -bmp_h;
    int row_padded = (bmp_w * (info->bpp/8) + 3) & (~3);
    
    
    int *x_map = kmalloc(screen_width * sizeof(int));
    for (int x = 0; x < screen_width; x++) {
        x_map[x] = (x * bmp_w) / screen_width;
    }
    
    
    for(int y = 0; y < screen_height; y++) {
        uint8_t *dst_row = (uint8_t*)wallpaper_buf + (y * screen_pitch);
        int src_y = (y * bmp_h) / screen_height;
        int src_y_offset = (bmp_h - 1 - src_y) * row_padded;
        
        for(int x = 0; x < screen_width; x++) {
            int src_offset = src_y_offset + (x_map[x] * (info->bpp/8));
            
            uint8_t b = pixels[src_offset], g = pixels[src_offset+1], r = pixels[src_offset+2];
            if (bpp_bytes == 4) {
                ((uint32_t*)dst_row)[x] = (r << 16) | (g << 8) | b;
            } else {
                dst_row[x*3] = b; dst_row[x*3+1] = g; dst_row[x*3+2] = r;
            }
        }
    }
    kfree(x_map);
    kfree_a(file);
    printf("[WM] Successfully loaded wallpaper: %s\n", path);
}

void sysconfig_reload(void) {
    int file_size = vfs_get_size("kernel.cfg");
    if(file_size <= 0) return;

    uint8_t *file_buffer = (uint8_t*)kmalloc_a(file_size + 1);
    vfs_read("kernel.cfg", file_buffer);
    file_buffer[file_size] = '\0';
    Config *cfg = config_parse((char *)file_buffer);
    char *v;

    if((v = config_get_value(cfg, "is_read_only_mode"))) isReadMode = (strcmp(v, "true") == 0) ? 1 : 0;
    if((v = config_get_value(cfg, "taskbar_color"))) taskbar_color = (uint32_t)atoi(v, 16);
    if((v = config_get_value(cfg, "window_border_color"))) window_border_color = (uint32_t)atoi(v, 16);
    if((v = config_get_value(cfg, "window_active_border_color"))) window_active_border_color = (uint32_t)atoi(v, 16);
    
    if((v = config_get_value(cfg, "key_kill"))) key_kill = (uint8_t)atoi(v, 16);
    if((v = config_get_value(cfg, "wm_gaps"))) wm_gaps = atoi(v, 10);
    
    if((v = config_get_value(cfg, "blur_radius"))) {
        blur_radius = atoi(v, 10);
        if(blur_radius < 1) blur_radius = 1;
        if(blur_radius > 32) blur_radius = 32; 
    }

    if((v = config_get_value(cfg, "font_scale"))) {
        font_scale = atoi(v, 10);
        if(font_scale < 1) font_scale = 1;
    }
    if((v = config_get_value(cfg, "wallpaper"))) {
        strcpy(wallpaper_path, v);
        printf("[WM] Parsing wallpaper from config: %s\n", wallpaper_path);
        load_wallpaper(wallpaper_path);
    }

    config_free(cfg);
    kfree_a(file_buffer);
    extern int taskbar_needs_update;
    taskbar_needs_update = 1;
    wm_refresh();
}

void sysconfig_init(void) { sysconfig_reload(); }