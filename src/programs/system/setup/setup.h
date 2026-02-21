#ifndef SETUP_H
#define SETUP_H

typedef struct{
    void (*main)(int, char**);
} Setup;

extern Setup setup;

#endif