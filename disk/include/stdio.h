#include "oslib.h"
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
int snprintf(char *str, size_t size, const char *format, ...);
int fprintf(FILE *stream, const char *format, ...);
int vfprintf(FILE *stream, const char *format, va_list args);
int fflush(FILE *stream);
int fputc(int c, FILE *stream);
int fputs(const char *s, FILE *stream);
FILE *fdopen(int fd, const char *mode);
FILE *freopen(const char *pathname, const char *mode, FILE *stream);
int remove(const char *pathname);
