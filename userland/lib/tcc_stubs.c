/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/lib/tcc_stubs.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include "oslib.h"
#include <time.h> 


int lseek(int fd, int offset, int whence) {
    int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(102), "b"(fd), "c"(offset), "d"(whence) : "memory"); return ret;
}

FILE *fdopen(int fd, const char *mode) {
    FILE *f = malloc(sizeof(FILE));
    if (f) f->fd = fd;
    return f;
}
int fputc(int c, FILE *stream) {
    unsigned char ch = c;
    if (stream && write(stream->fd, &ch, 1) == 1) return c;
    return EOF;
}
int fputs(const char *s, FILE *stream) {
    if (!stream) { printf("%s", s); return 0; }
    int len = strlen(s);
    if (write(stream->fd, s, len) == len) return 0;
    return EOF;
}
int fflush(FILE *stream) { return 0; }
int fprintf(FILE *stream, const char *format, ...) {
    char buf[1024];
    va_list args;
    va_start(args, format);
    int ret = vsprintf(buf, format, args);
    va_end(args);
    if (stream) write(stream->fd, buf, ret);
    else printf("%s", buf); 
    return ret;
}
int vfprintf(FILE *stream, const char *format, va_list args) {
    char buf[1024];
    int ret = vsprintf(buf, format, args);
    if (stream) write(stream->fd, buf, ret);
    return ret;
}
int remove(const char *pathname) {
    return delete_file(pathname) == 1 ? 0 : -1;
}


char *strpbrk(const char *s, const char *accept) {
    while (*s) {
        const char *a = accept;
        while (*a) {
            if (*s == *a) return (char *)s;
            a++;
        }
        s++;
    }
    return NULL;
}
int memcmp(const void *s1, const void *s2, uint32_t n) {
    const unsigned char *p1 = s1, *p2 = s2;
    while (n--) { if (*p1 != *p2) return *p1 - *p2; p1++; p2++; } return 0;
}

typedef struct header { struct header *ptr; unsigned int size; } Header;

void *realloc(void *ptr, uint32_t size) {
    if (!size) { free(ptr); return NULL; }
    if (!ptr) return malloc(size);
    void *new_ptr = malloc(size); 
    if (new_ptr) { 
        Header *bp = (Header *)ptr - 1;
        uint32_t old_size = (bp->size - 1) * sizeof(Header);
        uint32_t copy_size = (old_size < size) ? old_size : size;
        memcpy(new_ptr, ptr, copy_size); 
        free(ptr); 
    }
    return new_ptr;
}


void *mmap(void *addr, size_t length, int prot, int flags, int fd, int offset) {
    void* ptr = malloc(length);
    if (ptr) memset(ptr, 0, length);
    return ptr;
}
int munmap(void *addr, size_t length) { free(addr); return 0; }


int snprintf(char *str, uint32_t size, const char *format, ...) {
    va_list args; va_start(args, format);
    int r = vsprintf(str, format, args); va_end(args); return r;
}
int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
    return vsprintf(str, format, ap); 
}

long double ldexpl(long double x, int exp) { return x; }
void qsort(void *base, uint32_t nmemb, uint32_t size, int (*compar)(const void *, const void *)) {
    char *arr = base; char *tmp = malloc(size);
    for (uint32_t i = 0; i < nmemb; i++) {
        for (uint32_t j = i + 1; j < nmemb; j++) {
            if (compar(arr + i * size, arr + j * size) > 0) {
                memcpy(tmp, arr + i * size, size); memcpy(arr + i * size, arr + j * size, size); memcpy(arr + j * size, tmp, size);
            }
        }
    } free(tmp);
}

int unlink(const char *pathname) { return delete_file((char*)pathname) == 1 ? 0 : -1; }
char *strerror(int errnum) { return "Unknown Error"; }
int execvp(const char *file, char *const argv[]) { return spawn((char*)file, (char**)argv, NULL); }


int time(void *tloc) { return 0; }
struct tm *localtime(const void *timer) { static struct tm dummy = {0}; return &dummy; }
int gettimeofday(void *tv, void *tz) { return 0; }

char *realpath(const char *path, char *resolved_path) { return strcpy(resolved_path ? resolved_path : (char*)malloc(256), path); }
int dlclose(void *handle) { return 0; }

unsigned long long strtoull(const char *nptr, char **endptr, int base) {
    unsigned long long res = 0;
    while (*nptr == ' ' || *nptr == '\t' || *nptr == '\n' || *nptr == '\r') nptr++;
    int sign = 1;
    if (*nptr == '-') { sign = -1; nptr++; }
    else if (*nptr == '+') { nptr++; }

    if (base == 0) {
        if (*nptr == '0') {
            if (nptr[1] == 'x' || nptr[1] == 'X') { base = 16; nptr += 2; }
            else { base = 8; nptr++; }
        } else { base = 10; }
    } else if (base == 16) {
        if (*nptr == '0' && (nptr[1] == 'x' || nptr[1] == 'X')) nptr += 2;
    }

    while (*nptr) {
        int val = -1;
        if (*nptr >= '0' && *nptr <= '9') val = *nptr - '0';
        else if (*nptr >= 'a' && *nptr <= 'z') val = *nptr - 'a' + 10;
        else if (*nptr >= 'A' && *nptr <= 'Z') val = *nptr - 'A' + 10;
        if (val == -1 || val >= base) break;
        res = res * base + val;
        nptr++;
    }
    if (endptr) *endptr = (char*)nptr;
    return sign == 1 ? res : -res;
}
long strtol(const char *nptr, char **endptr, int base) { return (long)strtoull(nptr, endptr, base); }
unsigned long strtoul(const char *nptr, char **endptr, int base) { return (unsigned long)strtoull(nptr, endptr, base); }
long long strtoll(const char *nptr, char **endptr, int base) { return (long long)strtoull(nptr, endptr, base); }

double strtod(const char *nptr, char **endptr) { return 0.0; }
float strtof(const char *nptr, char **endptr) { return 0.0f; }


__attribute__((naked)) int setjmp(jmp_buf env) {
    __asm__ volatile (
        "mov 4(%esp), %ecx\n"
        "mov %ebx, 0(%ecx)\n"
        "mov %esi, 4(%ecx)\n"
        "mov %edi, 8(%ecx)\n"
        "mov %ebp, 12(%ecx)\n"
        "lea 4(%esp), %edx\n"
        "mov %edx, 16(%ecx)\n"
        "mov 0(%esp), %edx\n"
        "mov %edx, 20(%ecx)\n"
        "xor %eax, %eax\n"
        "ret\n"
    );
}

__attribute__((naked)) void longjmp(jmp_buf env, int val) {
    __asm__ volatile (
        "mov 4(%esp), %edx\n"
        "mov 8(%esp), %eax\n"
        "test %eax, %eax\n"
        "jnz 1f\n"
        "inc %eax\n"
        "1:\n"
        "mov 0(%edx), %ebx\n"
        "mov 4(%edx), %esi\n"
        "mov 8(%edx), %edi\n"
        "mov 12(%edx), %ebp\n"
        "mov 16(%edx), %esp\n"
        "jmp *20(%edx)\n"
    );
}

int errno = 0;


char *empty_environ[] = { NULL };
char **environ = empty_environ;


int mprotect(void *addr, size_t len, int prot) { return 0; }

FILE *freopen(const char *pathname, const char *mode, FILE *stream) { return stream; }


struct sigaction;
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact) { return 0; }
int sigemptyset(int *set) { return 0; }
int sigaddset(int *set, int signum) { return 0; }
int sigprocmask(int how, const int *set, int *oldset) { return 0; }
void *_GLOBAL_OFFSET_TABLE_[1] = {0};
long double strtold(const char *nptr, char **endptr) { 
    return 0.0; 
}

int rename(const char *oldpath, const char *newpath) {
    
    
    int sz = get_file_size(oldpath);
    if (sz < 0) return -1; 

    uint8_t *buf = NULL;
    if (sz > 0) {
        buf = malloc(sz);
        if (!buf) return -1; 
        read_file(oldpath, buf);
    }

    
    int res = write_file(newpath, buf, sz);
    if (buf) free(buf);

    if (res >= 0) {
        
        delete_file(oldpath);
        return 0;
    }
    
    return -1;
}

int puts(const char *s) {
    return printf("%s\n", s);
}

int putchar(int c) {
    return fputc(c, stdout);
}

int system(const char *command) {
    return -1; 
}

double atof(const char *nptr) {
    return (double)atoi(nptr);
}

int sscanf(const char *str, const char *format, ...) {
    
    if (format[0] == '%' && format[1] == 'x') {
        va_list args;
        va_start(args, format);
        unsigned int *val = va_arg(args, unsigned int *);
        *val = strtoul(str, NULL, 16); 
        va_end(args);
        return 1;
    }
    return 0; 
}
