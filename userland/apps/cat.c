#include "oslib.h"


#define CHUNK_SIZE 512

int main(int argc, char** argv) {
    if (argc < 2) { 
        printf("cat: missing file operand\n"); 
        return 1; 
    }
    
    for(int i = 1; i < argc; i++) {
        int total_size = get_file_size(argv[i]);
        if (total_size > 0) {
            
            
            if (total_size <= CHUNK_SIZE) {
                
                char buf[CHUNK_SIZE + 1]; 
                read_file(argv[i], (uint8_t*)buf);
                buf[total_size] = '\0'; 
                printf("%s", buf);
                if (buf[total_size-1] != '\n') printf("\n");
            } 
            else {
                
                
                
                
                
                
                
                
                
                
                
                uint8_t *safe_user_buffer = (uint8_t*)(0x40000000 + 0x20000); 
                
                read_file(argv[i], safe_user_buffer);
                
                
                int bytes_printed = 0;
                while (bytes_printed < total_size) {
                    int chunk = (total_size - bytes_printed > CHUNK_SIZE) ? CHUNK_SIZE : (total_size - bytes_printed);
                    
                    char print_buf[CHUNK_SIZE + 1];
                    for(int j = 0; j < chunk; j++) {
                        print_buf[j] = safe_user_buffer[bytes_printed + j];
                    }
                    print_buf[chunk] = '\0';
                    
                    printf("%s", print_buf);
                    bytes_printed += chunk;
                }
                
                if (safe_user_buffer[total_size-1] != '\n') printf("\n");
            }
        } else {
            printf("cat: %s: No such file or directory\n", argv[i]);
        }
    }
    return 0;
}