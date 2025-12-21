#ifndef SETUP_H
#define SETUP_H

typedef struct{
    int (*main)(void);
} Setup;

extern Setup setup;

#endif