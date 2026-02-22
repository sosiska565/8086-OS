#include "oslib.h"
#include "string_lib.h"

void main(int argc, char **argv){
    if(argc < 2 || strcmp(argv[1], "&") == 0){
        printf("Usage: exec fmcc.bin <file name>");
        return;
    }

    uint8_t file_buff[4096];

    int file_size = read_file(argv[1], file_buff);

    if(file_size <= 0){
        printf("File empty or not found");
        return;
    }
}