#ifndef MEMORY_VIEWER_H
#define MEMORY_VIEWER_H

#include <stdint.h>

typedef struct memory_viewer {
    int (*main)(void);
} memory_viewer_t;

extern memory_viewer_t memoryViewer;

#endif