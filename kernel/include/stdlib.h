#ifndef _KERNEL_STDLIB_H
#define _KERNEL_STDLIB_H



long atoi(const char *str, int base);




#define atoi(str) (int)atoi((str), 10)

#endif