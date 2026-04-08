#ifndef OSLIB_H
#define OSLIB_H

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>

#define NULL 0

#define COLOR_BLACK         0
#define COLOR_BLUE          1
#define COLOR_GREEN         2
#define COLOR_CYAN          3
#define COLOR_RED           4
#define COLOR_MAGENTA       5
#define COLOR_BROWN         6
#define COLOR_LIGHT_GRAY    7
#define COLOR_DARK_GRAY     8
#define COLOR_LIGHT_BLUE    9
#define COLOR_LIGHT_GREEN   10
#define COLOR_LIGHT_CYAN    11
#define COLOR_LIGHT_RED     12
#define COLOR_LIGHT_MAGENTA 13
#define COLOR_YELLOW        14
#define COLOR_WHITE         15

#define KEY_BACKSPACE '\b'
#define KEY_ENTER     '\n'
#define KEY_TAB       '\t'
#define KEY_CTRL_L    12
#define KEY_UP        17
#define KEY_DOWN      18
#define KEY_LEFT      19
#define KEY_RIGHT     20
#define KEY_F1        21
#define KEY_F2        22
#define KEY_ESC       27

#define VFS_ATTR_FILE 0
#define VFS_ATTR_DIR  1
#define VFS_ATTR_DEV  2

typedef struct {
    char name[64];
    uint32_t size;
    uint8_t type;
} vfs_dirent_t;

int read_file(char *file_name, uint8_t *file_buffer);
int write_file(char *filename, uint8_t *buffer, uint32_t size);
int get_file_size(char *file_name);
int delete_file(char *file_name);

int mkdir(char *path);
int readdir(char *path, int index, vfs_dirent_t *out);
int chdir(char *path);
int getcwd(char *buf);

int read(int fd, void *buf, uint32_t count);
int write(int fd, const void *buf, uint32_t count);

void set_color(int fg_color, int bg_color);
void clear_screen(void);
void get_cursor(int *x, int *y);
void set_cursor(int x, int y);
void get_term_size(int *cols, int *rows);

void printf(const char* format, ...);
void vprintf(const char* format, va_list args);
void sprintf(char *str, const char *format, ...);
void vsprintf(char *str, const char *format, va_list args);

void print(char *str);
void print_char(unsigned int c);

unsigned int getc(void);
void gets(char *buffer, int max_len);
void getpass(char *buffer, int max_len); 

typedef struct {
    int id;
    int parent_id;
    int state;
    char name[32];
} task_info_user_t;

int spawn(char* path, char** argv, char* redirect_out);
void waitpid(int pid);
void kill(int pid);
void exit(void);

int getuid(void);
int setuid(int uid);
int get_tasks(task_info_user_t* buffer, int max_count);

void *malloc(int size);
void free(void *ptr);
void get_mem_info(uint32_t* used, uint32_t* total);

int getenv(char* key, char* out_buf);

int strcmp(const char *s1, const char *s2);
int strlen(const char *s);
void strcpy(char *dst, const char *src);
void strcat(char *dst, const char *src);
int atoi(const char *str);


uint32_t hash_pw(const char* str);
int get_user_info(char* username, int* out_uid, uint32_t* out_hash);
int update_user(char* username, int new_uid, uint32_t new_hash, int delete_user);

uint32_t dlopen(char* lib_name);
uint32_t dlsym(uint32_t handle, char* symbol_name);

void memcpy(void *dest, const void* src, uint32_t n);
void memset(void* dest, uint32_t val, size_t count_pixels);

#endif