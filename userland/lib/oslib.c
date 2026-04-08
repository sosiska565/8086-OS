#include <oslib.h>

void print_char(unsigned int c) { char ch = (char)c; write(1, &ch, 1); }
void print(char *str) { write(1, str, strlen(str)); }

void vsprintf(char *str, const char *format, va_list args) {
    int pos = 0;
    while (*format) {
        if (*format == '%') {
            format++;
            if (*format == 'd' || *format == 'i') {
                int n = va_arg(args, int);
                char buf[16]; int i = 0, is_neg = 0;
                if (n == 0) { str[pos++] = '0'; }
                else {
                    if (n < 0) { is_neg = 1; n = -n; }
                    while (n > 0) { buf[i++] = (n % 10) + '0'; n /= 10; }
                    if (is_neg) buf[i++] = '-';
                    while (--i >= 0) str[pos++] = buf[i];
                }
            } else if (*format == 'u') {
                uint32_t n = va_arg(args, uint32_t);
                char buf[16]; int i = 0;
                if (n == 0) { str[pos++] = '0'; }
                else {
                    while (n > 0) { buf[i++] = (n % 10) + '0'; n /= 10; }
                    while (--i >= 0) str[pos++] = buf[i];
                }
            } else if (*format == 'x' || *format == 'X') {
                unsigned int n = va_arg(args, unsigned int);
                char buf[16]; int i = 0;
                if (n == 0) { str[pos++] = '0'; }
                else {
                    while (n > 0) { int rem = n % 16; buf[i++] = (rem < 10) ? (rem + '0') : (rem - 10 + 'A'); n /= 16; }
                    while (--i >= 0) str[pos++] = buf[i];
                }
            } else if (*format == 's') {
                char* s = va_arg(args, char*);
                if (!s) s = "(null)";
                while (*s) str[pos++] = *s++;
            } else if (*format == 'c') {
                str[pos++] = (char)va_arg(args, int);
            } else if (*format == '%') {
                str[pos++] = '%';
            }
        } else { str[pos++] = *format; }
        format++;
    }
    str[pos] = '\0';
}

void sprintf(char *str, const char *format, ...) { va_list args; va_start(args, format); vsprintf(str, format, args); va_end(args); }
void vprintf(const char* format, va_list args) { char buffer[1024]; vsprintf(buffer, format, args); print(buffer); }
void printf(const char* format, ...) { va_list args; va_start(args, format); vprintf(format, args); va_end(args); }

unsigned int getc(void) { char ch = 0; read(0, &ch, 1); return ch; }

void gets(char *buffer, int max_len) {
    int pos = 0;
    while(1) {
        char c = getc();
        if (c == KEY_ENTER) { buffer[pos] = '\0'; printf("\n"); break; }
        else if (c == KEY_BACKSPACE) { if (pos > 0) { pos--; buffer[pos] = '\0'; printf("\b \b"); } }
        else if (c >= 32 && c <= 126 && pos < max_len - 1) { buffer[pos++] = c; printf("%c", c); }
    }
}


void getpass(char *buffer, int max_len) {
    int pos = 0;
    while(1) {
        char c = getc();
        if (c == KEY_ENTER) { buffer[pos] = '\0'; printf("\n"); break; }
        else if (c == KEY_BACKSPACE) { if (pos > 0) { pos--; buffer[pos] = '\0'; printf("\b \b"); } }
        else if (c >= 32 && c <= 126 && pos < max_len - 1) { buffer[pos++] = c; printf("*"); }
    }
}

int read(int fd, void *buf, uint32_t count) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(3), "b"(fd), "c"(buf), "d"(count) : "memory"); return ret; }
int write(int fd, const void *buf, uint32_t count) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(4), "b"(fd), "c"(buf), "d"(count) : "memory"); return ret; }
int read_file(char *file_name, uint8_t *file_buffer) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(5), "b"(file_name), "c"(file_buffer) : "memory"); return ret; }
int write_file(char *filename, uint8_t *buffer, uint32_t size) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(6), "b"(filename), "c"(buffer), "d"(size) : "memory"); return ret; }
int get_file_size(char *file_name) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(8), "b"(file_name) : "memory"); return ret; }
int delete_file(char *file_name) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(10), "b"(file_name) : "memory"); return ret; }
int chdir(char *path) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(12), "b"(path) : "memory"); return ret; }
int getcwd(char *buf) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(79), "b"(buf) : "memory"); return ret; }
int readdir(char *path, int index, vfs_dirent_t *out) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(89), "b"(path), "c"(index), "d"(out) : "memory"); return ret; }
int mkdir(char *path) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(39), "b"(path) : "memory"); return ret; }
int spawn(char* path, char** argv, char* redirect_out) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(11), "b"(path), "c"(argv), "d"(redirect_out) : "memory"); return ret; }
void waitpid(int pid) { __asm__ volatile("int $0x80" : : "a"(7), "b"(pid)); }
void kill(int pid) { __asm__ volatile("int $0x80" : : "a"(37), "b"(pid)); }
void exit(void) { __asm__ volatile("int $0x80" : : "a"(1)); while(1); }
int getuid(void) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(20) : "memory"); return ret; }
int setuid(int uid) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(21), "b"(uid) : "memory"); return ret; }
int get_tasks(task_info_user_t* buffer, int max_count) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(14), "b"(buffer), "c"(max_count) : "memory"); return ret; }
void *malloc(int size) { void *ptr; __asm__ volatile("int $0x80" : "=a"(ptr) : "a"(45), "b"(size)); return ptr; }
void free(void *ptr) { __asm__ volatile("int $0x80" : : "a"(46), "b"(ptr)); }
void get_mem_info(uint32_t* used, uint32_t* total) { __asm__ volatile("int $0x80" : : "a"(15), "b"(used), "c"(total) : "memory"); }
int getenv(char* key, char* out_buf) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(19), "b"(key), "c"(out_buf) : "memory"); return ret; }
void set_color(int fg, int bg) { __asm__ volatile("int $0x80" : : "a"(13), "b"(fg), "c"(bg)); }
void get_cursor(int *x, int *y) { __asm__ volatile("int $0x80" : : "a"(16), "b"(x), "c"(y) : "memory"); }
void set_cursor(int x, int y) { __asm__ volatile("int $0x80" : : "a"(17), "b"(x), "c"(y)); }
void clear_screen(void) { __asm__ volatile("int $0x80" : : "a"(18)); }
void get_term_size(int *cols, int *rows) { __asm__ volatile("int $0x80" : : "a"(22), "b"(cols), "c"(rows) : "memory"); }

int strcmp(const char *s1, const char *s2) { while (*s1 && (*s1 == *s2)) { s1++; s2++; } return *(const unsigned char*)s1 - *(const unsigned char*)s2; }
int strlen(const char *s) { int i = 0; while(s[i]) i++; return i; }
void strcpy(char *dst, const char *src) { while(*src) *dst++ = *src++; *dst = '\0'; }
void strcat(char *dst, const char *src) { while(*dst) dst++; while(*src) *dst++ = *src++; *dst = '\0'; }
int atoi(const char *str) { int res=0; while(*str>='0' && *str<='9') { res=res*10+(*str-'0'); str++; } return res; }



uint32_t hash_pw(const char* str) {
    uint32_t hash = 2166136261u; 
    while (*str) {
        hash ^= (uint8_t)*str++;
        hash *= 16777619;
    }
    return hash;
}

int get_user_info(char* username, int* out_uid, uint32_t* out_hash) {
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

int update_user(char* username, int new_uid, uint32_t new_hash, int delete_user) {
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
                if (!delete_user) {
                    char tmp[128];
                    sprintf(tmp, "%s:%d:%u\n", username, new_uid, new_hash);
                    strcat(new_buf, tmp);
                }
            } else { strcat(new_buf, line); strcat(new_buf, "\n"); }
            line = end;
        }
    }
    
    if (!found && !delete_user) {
        char tmp[128];
        sprintf(tmp, "%s:%d:%u\n", username, new_uid, new_hash);
        strcat(new_buf, tmp);
    }
    
    write_file("/users.cfg", (uint8_t*)new_buf, strlen(new_buf));
    free(buf); free(new_buf); return 1;
}

uint32_t dlopen(char* lib_name) {
    uint32_t ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(90), "b"(lib_name) : "memory"); return ret;
}
uint32_t dlsym(uint32_t handle, char* symbol_name) {
    uint32_t ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(91), "b"(handle), "c"(symbol_name) : "memory"); return ret;
}

void memcpy(void *dest, const void* src, uint32_t n){
    uint32_t dwords = n / 4;
    uint32_t bytes = n % 4;
    __asm__ volatile(
        "rep movsl\n\t"
        "mov %3, %%ecx\n\t"
        "rep movsb"
        : "+D"(dest), "+S"(src), "+c"(dwords)
        : "r"(bytes)
        : "memory"
    );
}
void memset(void* dest, uint32_t val, size_t count_pixels) {
    __asm__ volatile ( "cld\n rep stosb" : : "a"(val), "D"(dest), "c"(count_pixels) : "memory" );
}