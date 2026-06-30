#ifndef _KERNEL_STDIO_H
#define _KERNEL_STDIO_H

#include <stdarg.h>

void printf(const char* format, ...);
void sprintf(char *str, const char *format, ...);
void vsprintf(char *str, const char *format, va_list args);



#define snprintf(buf, size, ...) sprintf(buf, __VA_ARGS__)

#endif