#include "oslib.h"
#define RTLD_DEFAULT ((void *)0)
#define RTLD_GLOBAL 0
#define RTLD_LAZY 0
int dlclose(void *handle);
