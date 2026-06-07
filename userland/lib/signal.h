#include "oslib.h"
#define SIGINT 2
#define SIGILL 4
#define SIGABRT 6
#define SIGFPE 8
#define SIGBUS 10
#define SIGSEGV 11
#define SA_SIGINFO 4
#define SIG_UNBLOCK 1
#define FPE_INTDIV 1
#define FPE_FLTDIV 2
typedef int sigset_t;
typedef void (*__sighandler_t)(int);
#define SIG_DFL ((__sighandler_t)0)
#define SIG_IGN ((__sighandler_t)1)
typedef struct { int si_signo; int si_code; } siginfo_t;
struct sigaction { void (*sa_sigaction)(int, siginfo_t *, void *); sigset_t sa_mask; int sa_flags; };
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
int sigemptyset(sigset_t *set);
int sigaddset(sigset_t *set, int signum);
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
