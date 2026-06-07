#include "oslib.h"

FILE _stdin_struct = {0}; FILE _stdout_struct = {1}; FILE _stderr_struct = {2};
FILE *stdin_ptr = &_stdin_struct; FILE *stdout_ptr = &_stdout_struct; FILE *stderr_ptr = &_stderr_struct;

void print_char(unsigned int c) { char ch = (char)c; write(1, &ch, 1); }
void print(const char *str) { write(1, str, strlen(str)); }

static int basic_strlen(const char *s) {
    int len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}

int vsprintf(char *str, const char *format, va_list args) {
    int pos = 0;
    while (*format) {
        if (*format == '%') {
            format++;
            if (*format == 'd' || *format == 'i') {
                int n = va_arg(args, int); char buf[16]; int i = 0, is_neg = 0;
                if (n == 0) { str[pos++] = '0'; }
                else {
                    if (n < 0) { is_neg = 1; n = -n; }
                    while (n > 0) { buf[i++] = (n % 10) + '0'; n /= 10; }
                    if (is_neg) buf[i++] = '-';
                    while (--i >= 0) str[pos++] = buf[i];
                }
            } else if (*format == 'u') {
                uint32_t n = va_arg(args, uint32_t); char buf[16]; int i = 0;
                if (n == 0) { str[pos++] = '0'; }
                else { while (n > 0) { buf[i++] = (n % 10) + '0'; n /= 10; } while (--i >= 0) str[pos++] = buf[i]; }
            } else if (*format == 'x' || *format == 'X') {
                unsigned int n = va_arg(args, unsigned int); char buf[16]; int i = 0;
                if (n == 0) { str[pos++] = '0'; }
                else { while (n > 0) { int rem = n % 16; buf[i++] = (rem < 10) ? (rem + '0') : (rem - 10 + 'A'); n /= 16; } while (--i >= 0) str[pos++] = buf[i]; }
            } else if (*format == 's') {
                char* s = va_arg(args, char*); if (!s) s = "(null)";
                while (*s) str[pos++] = *s++;
            } else if (*format == 'c') {
                str[pos++] = (char)va_arg(args, int);
            } else if (*format == '%') { str[pos++] = '%'; }
        } else { str[pos++] = *format; }
        format++;
    }
    str[pos] = '\0'; return pos;
}

int sprintf(char *str, const char *format, ...) { va_list args; va_start(args, format); int r = vsprintf(str, format, args); va_end(args); return r; }
int vprintf(const char* format, va_list args) { char buffer[1024]; int r = vsprintf(buffer, format, args); print(buffer); return r; }
int printf(const char* format, ...) { va_list args; va_start(args, format); int r = vprintf(format, args); va_end(args); return r; }

int getc(void) { char ch = 0; read(0, &ch, 1); return ch; }

void gets(char *buffer, int max_len) {
    int pos = 0;
    while(1) {
        char c = getc();
        if (c == KEY_ENTER) { buffer[pos] = '\0'; printf("\n"); break; }
        else if (c == KEY_BACKSPACE) { if (pos > 0) { pos--; buffer[pos] = '\0'; printf("\b \b"); } }
        else if (c >= 32 && c <= 126 && pos < max_len - 1) { buffer[pos++] = c; printf("%c", c); }
    }
}

int read(int fd, void *buf, uint32_t count) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(104), "b"(fd), "c"(buf), "d"(count) : "memory"); return ret; }
int write(int fd, const void *buf, uint32_t count) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(105), "b"(fd), "c"(buf), "d"(count) : "memory"); return ret; }

int read_file(const char *file_name, uint8_t *file_buffer) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(5), "b"(file_name), "c"(file_buffer) : "memory"); return ret; }
int write_file(const char *filename, uint8_t *buffer, uint32_t size) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(6), "b"(filename), "c"(buffer), "d"(size) : "memory"); return ret; }
int get_file_size(const char *file_name) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(8), "b"(file_name) : "memory"); return ret; }
int delete_file(const char *file_name) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(10), "b"(file_name) : "memory"); return ret; }
int chdir(const char *path) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(12), "b"(path) : "memory"); return ret; }
char *getcwd(char *buf, uint32_t size) { __asm__ volatile("int $0x80" : : "a"(79), "b"(buf) : "memory"); return buf; }
int readdir(const char *path, int index, vfs_dirent_t *out) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(89), "b"(path), "c"(index), "d"(out) : "memory"); return ret; }
int mkdir(const char *path) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(39), "b"(path) : "memory"); return ret; }
int spawn(const char* path, char** argv, const char* redirect_out) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(11), "b"(path), "c"(argv), "d"(redirect_out) : "memory"); return ret; }
void waitpid(int pid) { __asm__ volatile("int $0x80" : : "a"(7), "b"(pid)); }
int kill(int pid, int sig) { __asm__ volatile("int $0x80" : : "a"(37), "b"(pid)); return 0; }
void exit(int code) { __asm__ volatile("int $0x80" : : "a"(1)); while(1); }

void *sbrk(int incr) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(103), "b"(incr) : "memory"); return (void*)ret; }

typedef struct header { struct header *ptr; unsigned int size; } Header;
static Header base; static Header *freep = NULL;
void *malloc(unsigned int nbytes) {
    Header *p, *prevp; unsigned nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;
    if ((prevp = freep) == NULL) { base.ptr = freep = prevp = &base; base.size = 0; }
    for (p = prevp->ptr; ; prevp = p, p = p->ptr) {
        if (p->size >= nunits) {
            if (p->size == nunits) prevp->ptr = p->ptr;
            else { p->size -= nunits; p += p->size; p->size = nunits; }
            freep = prevp; return (void *)(p + 1);
        }
        if (p == freep) {
            unsigned alloc_units = (nunits > 1024) ? nunits : 1024;
            char *cp = sbrk(alloc_units * sizeof(Header));
            if (cp == (char *)-1) return NULL;
            Header *up = (Header *)cp; up->size = alloc_units; free((void *)(up + 1)); p = freep;
        }
    }
}

char **parse_str(const char *term_input, char delimiter) {
    if (term_input == NULL) return NULL;

    int len = basic_strlen(term_input);
    int words = 0;
    int in_word = 0;

    
    for (int i = 0; i <= len; i++) {
        if (term_input[i] == delimiter || term_input[i] == '\0') {
            if (in_word) {
                words++;
                in_word = 0;
            }
        } else {
            in_word = 1;
        }
    }

    
    char **result = (char **)malloc((words + 1) * sizeof(char *));
    if (result == NULL) return NULL;

    int word_index = 0;
    int word_start = -1;

    
    for (int i = 0; i <= len; i++) {
        if (term_input[i] == delimiter || term_input[i] == '\0') {
            if (word_start != -1) {
                int word_len = i - word_start;
                
                
                result[word_index] = (char *)malloc((word_len + 1) * sizeof(char));
                if (result[word_index] != NULL) {
                    
                    for (int j = 0; j < word_len; j++) {
                        result[word_index][j] = term_input[word_start + j];
                    }
                    result[word_index][word_len] = '\0'; 
                    word_index++;
                }
                word_start = -1;
            }
        } else {
            if (word_start == -1) {
                word_start = i;
            }
        }
    }

    result[word_index] = NULL; 
    return result;
}

void free(void *ap) {
    Header *bp, *p; if (ap == NULL) return; bp = (Header *)ap - 1;
    for (p = freep; !(bp > p && bp < p->ptr); p = p->ptr) { if (p >= p->ptr && (bp > p || bp < p->ptr)) break; }
    if (bp + bp->size == p->ptr) { bp->size += p->ptr->size; bp->ptr = p->ptr->ptr; } else bp->ptr = p->ptr;
    if (p + p->size == bp) { p->size += bp->size; p->ptr = bp->ptr; } else p->ptr = bp;
    freep = p;
}

FILE *fopen(const char *path, const char *mode) {
    int flags = 0;
    if (mode[0] == 'r') flags = O_RDONLY; else if (mode[0] == 'w') flags = O_WRONLY | O_CREAT | O_TRUNC; else if (mode[0] == 'a') flags = O_WRONLY | O_CREAT;
    int fd; __asm__ volatile("int $0x80" : "=a"(fd) : "a"(100), "b"(path), "c"(flags) : "memory");
    if (fd < 0) return NULL; FILE *f = malloc(sizeof(FILE)); f->fd = fd;
    if (mode[0] == 'a') fseek(f, 0, SEEK_END); return f;
}
int fclose(FILE *stream) { if (!stream) return -1; int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(101), "b"(stream->fd) : "memory"); free(stream); return ret; }
int fread(void *ptr, uint32_t size, uint32_t nmemb, FILE *stream) { if (!stream) return 0; int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(104), "b"(stream->fd), "c"(ptr), "d"(size * nmemb) : "memory"); if (ret < 0) return 0; return ret / size; }
int fwrite(const void *ptr, uint32_t size, uint32_t nmemb, FILE *stream) { if (!stream) return 0; int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(105), "b"(stream->fd), "c"(ptr), "d"(size * nmemb) : "memory"); if (ret < 0) return 0; return ret / size; }
int fseek(FILE *stream, int offset, int whence) { if (!stream) return -1; int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(102), "b"(stream->fd), "c"(offset), "d"(whence) : "memory"); return (ret >= 0) ? 0 : -1; }
int ftell(FILE *stream) { if (!stream) return -1; int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(102), "b"(stream->fd), "c"(0), "d"(SEEK_CUR) : "memory"); return ret; }
int fgetc(FILE *stream) { unsigned char c; if (fread(&c, 1, 1, stream) == 1) return c; return EOF; }

static char _env_buf[128];
char *getenv(const char* key) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(19), "b"(key), "c"(_env_buf) : "memory"); if (ret) return _env_buf; return NULL; }

int strcmp(const char *s1, const char *s2) { while (*s1 && (*s1 == *s2)) { s1++; s2++; } return *(const unsigned char*)s1 - *(const unsigned char*)s2; }
int strncmp(const char *s1, const char *s2, int n) { while (n > 0 && *s1 && (*s1 == *s2)) { ++s1; ++s2; --n; } if (n == 0) return 0; return (*(unsigned char *)s1 - *(unsigned char *)s2); }
int strlen(const char *s) { int i = 0; while(s[i]) i++; return i; }
char *strcpy(char *dst, const char *src) { char* ret = dst; while(*src) *dst++ = *src++; *dst = '\0'; return ret; }
char *strcat(char *dst, const char *src) { char* ret = dst; while(*dst) dst++; while(*src) *dst++ = *src++; *dst = '\0'; return ret; }
int atoi(const char *str) { int res=0; while(*str>='0' && *str<='9') { res=res*10+(*str-'0'); str++; } return res; }

uint32_t hash_pw(const char* str) { uint32_t hash = 2166136261u; while (*str) { hash ^= (uint8_t)*str++; hash *= 16777619; } return hash; }

int get_user_info(const char* username, int* out_uid, uint32_t* out_hash) {
    int sz = get_file_size("/users.cfg");
    if(sz <= 0) return 0;
    char* buf = malloc(sz+1);
    read_file("/users.cfg", (uint8_t*)buf);
    buf[sz] = 0;

    char* line = buf;
    while(*line) {
        char* end = line;
        while(*end && *end != '\n') end++;
        if(*end) { *end = 0; end++; }

        char u[32], id_s[16], h_s[32];
        int i=0, j=0;
        while(line[i] && line[i]!=':') u[j++] = line[i++]; u[j]=0; if(line[i]==':') i++;
        j=0; while(line[i] && line[i]!=':') id_s[j++] = line[i++]; id_s[j]=0; if(line[i]==':') i++;
        j=0; while(line[i]) h_s[j++] = line[i++]; h_s[j]=0;

        if(strcmp(u, username) == 0) {
            *out_uid = atoi(id_s);
            uint32_t res = 0; char* p = h_s;
            while(*p>='0' && *p<='9') { res=res*10+(*p-'0'); p++; }
            *out_hash = res;
            free(buf); return 1;
        }
        line = end;
    }
    free(buf); return 0;
}

int update_user(const char* username, int new_uid, uint32_t new_hash, int delete_user) {
    int sz = get_file_size("/users.cfg");
    char* buf = malloc((sz > 0 ? sz : 0) + 512);
    char* new_buf = malloc((sz > 0 ? sz : 0) + 512);
    new_buf[0] = 0;
    int found = 0;
    
    if (sz > 0) {
        read_file("/users.cfg", (uint8_t*)buf);
        buf[sz] = 0;
        char* line = buf;
        while(*line) {
            char* end = line;
            while(*end && *end != '\n') end++;
            if(*end) { *end = 0; end++; }
            
            char u[32], id_s[16], h_s[32];
            int i=0, j=0;
            while(line[i] && line[i]!=':') u[j++] = line[i++]; u[j]=0; if(line[i]==':') i++;
            j=0; while(line[i] && line[i]!=':') id_s[j++] = line[i++]; id_s[j]=0; if(line[i]==':') i++;
            j=0; while(line[i]) h_s[j++] = line[i++]; h_s[j]=0;
            
            if (strcmp(u, username) == 0) {
                found = 1;
                if (!delete_user) { char tmp[128]; sprintf(tmp, "%s:%d:%u\n", username, new_uid, new_hash); strcat(new_buf, tmp); }
            } else { strcat(new_buf, line); strcat(new_buf, "\n"); }
            line = end;
        }
    }
    
    if (!found && !delete_user) { char tmp[128]; sprintf(tmp, "%s:%d:%u\n", username, new_uid, new_hash); strcat(new_buf, tmp); }
    write_file("/users.cfg", (uint8_t*)new_buf, strlen(new_buf));
    free(buf); free(new_buf); return 1;
}

void *dlopen(const char* lib_name, int flags) { uint32_t ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(90), "b"(lib_name) : "memory"); return (void*)ret; }
void *dlsym(void* handle, const char* symbol_name) { uint32_t ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(91), "b"((uint32_t)handle), "c"(symbol_name) : "memory"); return (void*)ret; }

void *memcpy(void *dest, const void* src, uint32_t n){ uint32_t dwords = n / 4; uint32_t bytes = n % 4; __asm__ volatile( "rep movsl\n\tmov %3, %%ecx\n\trep movsb" : "+D"(dest), "+S"(src), "+c"(dwords) : "r"(bytes) : "memory" ); return dest; }
void *memset(void* dest, int val, uint32_t count_pixels) { __asm__ volatile ( "cld\n rep stosb" : : "a"(val), "D"(dest), "c"(count_pixels) : "memory" ); return dest; }

int mount(const char* device, const char* mount_point, const char* fs_type) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(92), "b"(device), "c"(mount_point), "d"(fs_type) : "memory"); return ret; }
int unmount(const char* mount_point) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(93), "b"(mount_point) : "memory"); return ret; }

void set_color(int fg, int bg) { __asm__ volatile("int $0x80" : : "a"(13), "b"(fg), "c"(bg)); }
void get_cursor(int *x, int *y) { __asm__ volatile("int $0x80" : : "a"(16), "b"(x), "c"(y) : "memory"); }
void set_cursor(int x, int y) { __asm__ volatile("int $0x80" : : "a"(17), "b"(x), "c"(y)); }
void clear_screen(void) { __asm__ volatile("int $0x80" : : "a"(18)); }
void get_term_size(int *cols, int *rows) { __asm__ volatile("int $0x80" : : "a"(22), "b"(cols), "c"(rows) : "memory"); }
int getuid(void) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(20) : "memory"); return ret; }
int setuid(int uid) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(21), "b"(uid) : "memory"); return ret; }
int get_tasks(task_info_user_t* buffer, int max_count) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(14), "b"(buffer), "c"(max_count) : "memory"); return ret; }
void get_mem_info(uint32_t* used, uint32_t* total) { __asm__ volatile("int $0x80" : : "a"(15), "b"(used), "c"(total) : "memory"); }

void detach(void) { __asm__ volatile("int $0x80" : : "a"(24)); }
void get_screen_info(int *w, int *h, int *bpp) { __asm__ volatile("int $0x80" : : "a"(25), "b"(w), "c"(h), "d"(bpp)); }
void flush_screen(void *buffer) { __asm__ volatile("int $0x80" : : "a"(26), "b"(buffer)); }
void get_mouse(int *x, int *y, int *buttons) { __asm__ volatile("int $0x80" : : "a"(27), "b"(x), "c"(y), "d"(buttons)); }
unsigned int poll_key(void) { unsigned int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(28)); return ret; }
uint8_t get_key_modifiers(void) { uint32_t ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(29)); return (uint8_t)ret; }
void yield() { __asm__ volatile("int $0x80" : : "a"(30)); }
uint32_t get_ticks() { 
    uint32_t ret; 
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(31)); 
    return ret; 
}
int get_key_state(uint8_t scancode) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(32), "b"(scancode));
    return ret;
}