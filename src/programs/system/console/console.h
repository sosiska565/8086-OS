#ifndef CONSOLE_H
#define CONSOLE_H

#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "programs/system/console/system.h"
#include "memory/memory.h"
#include "programs/system/memory_viewer/memory_viewer.h"

typedef struct console{
    int (*main)(void);
} Console;

extern Console console;

#endif