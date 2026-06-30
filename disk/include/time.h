/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/lib/time.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include "oslib.h"
struct tm { int tm_sec; int tm_min; int tm_hour; int tm_mday; int tm_mon; int tm_year; int tm_wday; int tm_yday; int tm_isdst; };
int time(void *tloc);
struct tm *localtime(const void *timer);
int gettimeofday(void *tv, void *tz);
