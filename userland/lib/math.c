/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/lib/math.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include "math.h"

double sqrt(double x) {
    double res;
    __asm__ volatile ("fsqrt" : "=t" (res) : "0" (x));
    return res;
}

double cos(double x) {
    double res;
    __asm__ volatile ("fcos" : "=t" (res) : "0" (x));
    return res;
}

double acos(double x) {
    double res;
    
    __asm__ volatile (
        "fld %%st(0)\n\t"           
        "fmul %%st(0), %%st(0)\n\t" 
        "fld1\n\t"                  
        "fsubrp\n\t"                
        "fsqrt\n\t"                 
        "fxch %%st(1)\n\t"          
        "fpatan\n\t"                
        : "=t" (res) : "0" (x) : "st(1)"
    );
    return res;
}

double floor(double x) {
    int i = (int)x;
    return (x < i) ? (double)(i - 1) : (double)i;
}

double ceil(double x) {
    int i = (int)x;
    return (x > i) ? (double)(i + 1) : (double)i;
}

double fmod(double x, double y) {
    if (y == 0.0) return 0.0;
    int quotient = (int)(x / y);
    return x - (quotient * y);
}

double pow(double x, double y) {
    if (x == 0.0) return 0.0;
    if (y == 0.0) return 1.0;
    
    double res;
    
    __asm__ volatile (
        "fyl2x\n\t"                  
        "fld %%st(0)\n\t"            
        "frndint\n\t"                
        "fsubr %%st(1), %%st(0)\n\t" 
        "f2xm1\n\t"                  
        "fld1\n\t"                   
        "faddp\n\t"                  
        "fscale\n\t"                 
        "fstp %%st(1)\n\t"           
        : "=t" (res) : "0" (x), "u" (y) : "st(1)"
    );
    return res;
}
