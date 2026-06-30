/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/system_apps/console/console.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#ifndef CONSOLE_H
#define CONSOLE_H

#include <stddef.h>

typedef struct {
    const char *name;
    void (*handler)(char **);
    const char *description;
} command_t;

typedef struct {
    int (*main)(void);
    int should_exit;
} Console;

extern command_t commands[]; 

void execute_command(char **tokens);

extern Console console;

#endif
