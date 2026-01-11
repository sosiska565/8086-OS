#include "oslib.h"
#include "strings.h"

void main(int argc, char **argv){
    print("not now :)\n");
    return;

    if(argv[1] == 0){
        print("Usage: rasm.bin <file>\n");
        return;
    }

    int filesize = get_file_size(argv[1]);
    if(filesize <= 0){
        print("File is not found or empty.\n");
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

    print(lines_buffer[1]);

    free(buffer);
}