/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/lib/math.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#ifndef _MATH_H
#define _MATH_H

#include "oslib.h"

long double ldexpl(long double x, int exp);

static inline double fabs(double x) {
    return (x < 0.0) ? -x : x;
}


double sqrt(double x);
double floor(double x);
double ceil(double x);
double fmod(double x, double y);
double pow(double x, double y);
double cos(double x);
double acos(double x);

#endif 
