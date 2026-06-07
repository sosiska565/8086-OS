#ifndef _TCCDEFS_H
#define _TCCDEFS_H
#define __SIZE_TYPE__ unsigned int
#define __PTRDIFF_TYPE__ int
#define __WCHAR_TYPE__ int
#define __builtin_va_list void*
#define __builtin_va_start(v,l) (v = (void*)((char*)&(l) + ((sizeof(l)+3)&~3)))
#define __builtin_va_arg(v,l)   (*(l*)(((char*)(v) += ((sizeof(l)+3)&~3)) - ((sizeof(l)+3)&~3)))
#define __builtin_va_end(v)     (v = (void*)0)
#endif