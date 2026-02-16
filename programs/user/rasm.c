#include "oslib.h"
#include "string_lib.h"

void main(int argc, char **argv){
    if(argv[1] == 0){
        printf("Usage: rasm.bin <file> <output file name>\n");
        return;
    }

    int filesize = get_file_size(argv[1]);
    if(filesize <= 0){
        printf("File is not found or empty.\n");
        return;
    }
    uint8_t *buffer = malloc(filesize + 1);
    read_file(argv[1], buffer);
    buffer[filesize] = '\0';

    char **lines_buffer;
    int lines_count = 1;
    for(int i = 0; i < filesize; i++){
        if(buffer[i] == '\n'){
            lines_count++;
        }
    }

    printf(lines_buffer[1]);

    //write_file(argv[2], (uint8_t*){0xCD, 0x80}, 2);

    free(buffer);
}