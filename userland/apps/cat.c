#include "oslib.h"

int main(int argc, char** argv) {
    if (argc < 2) { printf("cat: missing file operand\n"); return 1; }
    
    for(int i = 1; i < argc; i++) {
        int size = get_file_size(argv[i]);
        if (size > 0) {
            char* buf = malloc(size + 1);
            read_file(argv[i], (uint8_t*)buf);
            buf[size] = '\0'; 
            printf(buf);
            if (buf[size-1] != '\n') printf("\n");
            free(buf);
        } else {
            printf("cat: "); printf(argv[i]); printf(": No such file or directory\n");
        }
    }
    return 0;
}