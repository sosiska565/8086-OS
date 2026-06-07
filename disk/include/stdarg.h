#include "oslib.h"
#ifndef va_copy
#define va_copy(d,s) __builtin_va_copy(d,s)
#endif
