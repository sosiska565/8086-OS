#ifndef GLOBAL_H
#define GLOBAL_H

#define OS_NAME          "8086-OS"
#define OS_VERSION_MAJOR 0
#define OS_VERSION_MINOR 9
#define OS_VERSION_PATCH 1
#define OS_VERSION_EXTRA "beta"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define OS_RELEASE STR(OS_VERSION_MAJOR) "." STR(OS_VERSION_MINOR) "." STR(OS_VERSION_PATCH) "-" OS_VERSION_EXTRA

#include <stdint.h>

extern int $;
extern unsigned short isReadMode;
extern char* path;

extern char wallpaper_path[128];
extern uint32_t *wallpaper_buf;
extern int font_scale;

#endif