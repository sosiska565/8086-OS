#ifndef LIB_H
#define LIB_H
#include <stdint.h>
#include <stdarg.h>

#define NULL 0
#define VFS_ATTR_FILE 0
#define VFS_ATTR_DIR  1
#define VFS_ATTR_DEV  2

typedef struct { char name[64]; uint32_t size; uint8_t type; } vfs_dirent_t;
typedef struct { int id; int parent_id; int state; char name[32]; } task_info_user_t;

int read(int fd, void *buf, uint32_t count);
int write(int fd, const void *buf, uint32_t count);
void exit(void);


void vsprintf(char *str, const char *format, va_list args);
void sprintf(char *str, const char *format, ...);
void vprintf(const char* format, va_list args);
void printf(const char* format, ...);

void print_char(unsigned int c);
void print(char *str);
unsigned int getc(void);
void gets(char *buffer, int max_len); 

void *malloc(int size);
void free(void *ptr);
int read_file(char *file_name, uint8_t *file_buffer);
int write_file(char *filename, uint8_t *buffer, uint32_t size);
int get_file_size(char *file_name);
int delete_file(char *file_name);

int spawn(char* path, char** argv, char* redirect_out); 
void waitpid(int pid);
void kill(int pid);

int getuid(void);
int setuid(int uid);

int chdir(char *path);
int getcwd(char *buf);
int readdir(char *path, int index, vfs_dirent_t *out);
int mkdir(char *path);

void set_color(int fg, int bg);
int get_tasks(task_info_user_t* buffer, int max_count);
void get_mem_info(uint32_t* used, uint32_t* total);
void get_cursor(int *x, int *y);
void set_cursor(int x, int y);
void clear_screen(void);
int getenv(char* key, char* out_buf);

int strcmp(const char *s1, const char *s2);
int strlen(const char *s);
void strcpy(char *dst, const char *src);
void strcat(char *dst, const char *src);
int atoi(const char *str);
int getuid(void);
int setuid(int uid);
int spawn(char* path, char** argv, char* redirect_out);

#endif