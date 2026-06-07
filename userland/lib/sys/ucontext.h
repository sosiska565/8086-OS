#include "oslib.h"
#define REG_EBP 6
#define REG_EIP 14
typedef struct { int gregs[19]; } mcontext_t;
typedef struct { mcontext_t uc_mcontext; } ucontext_t;
