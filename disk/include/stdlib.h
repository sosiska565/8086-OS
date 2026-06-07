#include "oslib.h"
extern char **environ;
void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));
long strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);
long long strtoll(const char *nptr, char **endptr, int base);
unsigned long long strtoull(const char *nptr, char **endptr, int base);
double strtod(const char *nptr, char **endptr);
float strtof(const char *nptr, char **endptr);
long double strtold(const char *nptr, char **endptr);
void *realloc(void *ptr, size_t size);
char *realpath(const char *path, char *resolved_path);
