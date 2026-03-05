#ifndef DISK_VIEWER_H
#define DISK_VIEWER_H

typedef struct {
    int (*main)(void);
} Disk_viewer;

extern Disk_viewer disk_viewer;

#endif