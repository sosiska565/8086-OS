/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/drivers/timer/timer.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void timer_handler_c(uint32_t esp);
unsigned long get_ticks(void);
void timer_install(void);
void sleep(unsigned long ms);
uint32_t get_cpu_usage(void);

#endif
