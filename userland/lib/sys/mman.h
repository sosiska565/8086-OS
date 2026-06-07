#include "oslib.h"
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4
#define MAP_PRIVATE 2
#define MAP_ANONYMOUS 0x20
#define MAP_FAILED ((void *)-1)
void *mmap(void *a, size_t l, int p, int f, int d, int o);
int munmap(void *a, size_t l);
int mprotect(void *addr, size_t len, int prot);
