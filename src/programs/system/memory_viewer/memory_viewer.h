#ifndef MEMORY_VIEWER_H
#define MEMORY_VIEWER_H

#include <stdint.h>
#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"

typedef struct memory_viewer {
    int (*main)(void);
} memory_viewer_t;

extern memory_viewer_t memory_viewer;

#endif