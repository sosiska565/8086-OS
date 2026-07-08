/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/lib/oslib.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#ifndef OSLIB_H
#define OSLIB_H

typedef unsigned int       uint32_t;
typedef int                int32_t;
typedef unsigned short     uint16_t;
typedef short              int16_t;
typedef unsigned char      uint8_t;
typedef signed char        int8_t;
typedef unsigned int       size_t;
typedef int                ssize_t;
typedef unsigned long long uint64_t;
typedef long long          int64_t;
typedef unsigned int       uintptr_t; 
typedef int                time_t;
typedef int                intptr_t;

typedef int jmp_buf[6]; 
int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);

#define NULL ((void*)0)
#define EOF (-1)

typedef __builtin_va_list va_list;
#define va_start(v,l)   __builtin_va_start(v,l)
#define va_end(v)       __builtin_va_end(v)
#define va_arg(v,l)     __builtin_va_arg(v,l)

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#define O_CREAT  0x0200
#define O_TRUNC  0x0400

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct { int fd; } FILE;

typedef struct {
  char sysname[65];  
  char release[65];  
  char version[65];  
  char machine[65];  
} os_version_info;

extern FILE *stdin_ptr;
extern FILE *stdout_ptr;
extern FILE *stderr_ptr;

#define stdin stdin_ptr
#define stdout stdout_ptr
#define stderr stderr_ptr

#define COLOR_BLACK 0
#define COLOR_BLUE 1
#define COLOR_GREEN 2
#define COLOR_CYAN 3
#define COLOR_RED 4
#define COLOR_MAGENTA 5
#define COLOR_BROWN 6
#define COLOR_LIGHT_GRAY 7
#define COLOR_DARK_GRAY 8
#define COLOR_LIGHT_BLUE 9
#define COLOR_LIGHT_GREEN 10
#define COLOR_LIGHT_CYAN 11
#define COLOR_LIGHT_RED 12
#define COLOR_LIGHT_MAGENTA 13
#define COLOR_YELLOW 14
#define COLOR_WHITE 15

#ifndef DOOM_PORT
#define KEY_BACKSPACE '\b'
#define KEY_ENTER '\n'
#define KEY_TAB '\t'
#define KEY_CTRL_L 12
#define KEY_UP 17
#define KEY_DOWN 18
#define KEY_LEFT 19
#define KEY_RIGHT 20
#define KEY_F1 21
#define KEY_F2 22
#define KEY_ESC 27
char *strdup(const char *s);
#endif

#define VFS_ATTR_FILE 0
#define VFS_ATTR_DIR 1
#define VFS_ATTR_DEV 2

#define SCANCODE_UP    0x48
#define SCANCODE_DOWN  0x50
#define SCANCODE_LEFT  0x4B
#define SCANCODE_RIGHT 0x4D
#define SCANCODE_ENTER 0x1C
#define SCANCODE_ESC   0x01
#define SCANCODE_CTRL  0x1D
#define SCANCODE_ALT   0x38
#define SCANCODE_SPACE 0x39
#define SCANCODE_W     0x11
#define SCANCODE_A     0x1E
#define SCANCODE_S     0x1F
#define SCANCODE_D     0x20
#define SCANCODE_LSHIFT 0x2A
#define SCANCODE_RSHIFT 0x36

typedef struct { char name[64]; uint32_t size; uint8_t type; } vfs_dirent_t;
typedef struct { int id; int parent_id; int state; char name[32]; } task_info_user_t;

int read_file(const char *file_name, uint8_t *file_buffer);
int write_file(const char *filename, uint8_t *buffer, uint32_t size);
int get_file_size(const char *file_name);
int delete_file(const char *file_name);

int open(const char *pathname, int flags, ...);
int close(int fd);

FILE *fopen(const char *path, const char *mode);
int fclose(FILE *stream);
int fread(void *ptr, uint32_t size, uint32_t nmemb, FILE *stream);
int fwrite(const void *ptr, uint32_t size, uint32_t nmemb, FILE *stream);
int fseek(FILE *stream, int offset, int whence);
int ftell(FILE *stream);
int fgetc(FILE *stream);

int mkdir(const char *path, ...);
int readdir(const char *path, int index, vfs_dirent_t *out);
int chdir(const char *path);
char *getcwd(char *buf, uint32_t size);

int mount(const char* device, const char* mount_point, const char* fs_type);
int unmount(const char* mount_point);

int read(int fd, void *buf, uint32_t count);
int write(int fd, const void *buf, uint32_t count);

int pipe(int pipefd[2]);
int spawn_ext(const char* path, char** argv, int fd_in, int fd_out);

void set_color(int fg_color, int bg_color);
void clear_screen(void);
void get_cursor(int *x, int *y);
void set_cursor(int x, int y);
void get_term_size(int *cols, int *rows);

int printf(const char* format, ...);
int vprintf(const char* format, va_list args);
int sprintf(char *str, const char *format, ...);
int vsprintf(char *str, const char *format, va_list args);

void print(const char *str);
void print_char(unsigned int c);

char getc(void);
void gets(char *buffer, int max_len);

int spawn(const char* path, char** argv, const char* redirect_out);
void waitpid(int pid);
int kill(int pid, int sig);
void exit(int code);

int getuid(void);
int setuid(int uid);
int get_tasks(task_info_user_t* buffer, int max_count);

void *malloc(unsigned int size);
char **parse_str(const char *term_input, char delimiter);
void free(void *ptr);
void *sbrk(int incr);
void get_mem_info(uint32_t* used, uint32_t* total);

char *getenv(const char* key);

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, int n); 
int strlen(const char *s);
char *strcpy(char *dst, const char *src);
char *strcat(char *dst, const char *src);
int atoi(const char *str);

uint32_t hash_pw(const char* str);
int get_user_info(const char* username, int* out_uid, uint32_t* out_hash);
int update_user(const char* username, int new_uid, uint32_t new_hash, int delete_user);

void *dlopen(const char* lib_name, int flags);
void *dlsym(void *handle, const char* symbol_name);

void *memcpy(void *dest, const void* src, uint32_t n);
void *memset(void* dest, int val, uint32_t count_pixels);

void detach_pid(int pid);
void get_screen_info(int *w, int *h, int *bpp);
void flush_screen(void *buffer);
void get_mouse(int *x, int *y, int *buttons, int *z);
unsigned int poll_key(void);
uint8_t get_key_modifiers(void);
void yield();

uint32_t get_ticks(void);
int get_key_state(uint8_t scancode);
int shm_get(int key, int size);
void* shm_map(int key);



#define AF_INET      2
#define SOCK_STREAM  1
#define SOCK_DGRAM   2
#define SOCK_RAW     3
#define IPPROTO_ICMP 1

typedef uint32_t in_addr_t;

struct in_addr {
  in_addr_t s_addr;
};

struct sockaddr_in {
  uint8_t         sin_len;
  uint8_t         sin_family;
  uint16_t        sin_port;
  struct in_addr  sin_addr;
  char            sin_zero[8];
};

struct sockaddr {
  uint8_t         sa_len;
  uint8_t         sa_family;
  char            sa_data[14];
};

int socket(int domain, int type, int protocol);
int connect(int fd, const struct sockaddr *addr, uint32_t addrlen);
int send(int fd, const void *data, uint32_t size, int flags);
int recv(int fd, void *mem, uint32_t len, int flags);

uint16_t htons(uint16_t n);
uint32_t inet_addr(const char *cp);

uint32_t gethostbyname(const char *name);

char *strstr(const char *haystack, const char *needle);

char *strchr(const char *s, int c);
char *strncpy(char *dest, const char *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
char *strrchr(const char *s, int c);
char *to_upper(char *s);
char *to_lower(char *s);

char scancode_to_char(uint8_t sc);
uint8_t wait_scancode();

void *calloc(uint32_t nmemb, uint32_t size);

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} rtc_time_t;

void get_rtc_time(rtc_time_t *t);

int kill(int pid, int sig);

int uname(os_version_info *buf);
void memset32(void *dest, uint32_t val, size_t count);

char *strdup(const char *s);
void flush_keyboard();

#endif
