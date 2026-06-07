#include "oslib.h"
extern char **environ;
int unlink(const char *pathname);
int close(int fd);
int lseek(int fd, int offset, int whence);
int execvp(const char *file, char *const argv[]);
