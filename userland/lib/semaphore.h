#include "oslib.h"
typedef int sem_t;
#define sem_init(s, p, v) (*(s)=(v),0)
#define sem_wait(s) (0)
#define sem_post(s) (0)
