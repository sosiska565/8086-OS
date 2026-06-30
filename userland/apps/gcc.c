#include <oslib.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        set_color(COLOR_RED, COLOR_BLACK);
        printf("Error: Missing input file.\n");
        set_color(COLOR_WHITE, COLOR_BLACK);
        printf("Usage: gcc <source_file.c>\n");
        return 1;
    }

    char* source_file = argv[1];
    
    char out_file[64];
    strcpy(out_file, source_file);
    int len = strlen(out_file);
    if (len > 2 && out_file[len-2] == '.' && out_file[len-1] == 'c') {
        out_file[len-2] = '\0';
    }
    strcat(out_file, ".elf");

    printf("Compiling %s -> %s...\n", source_file, out_file);

    
    char* tcc_args[] = {
        "tcc", 
        "-nostdlib", 
        "-I/include", 
        "-Wl,-Ttext=0x60000000", 
        "-o", out_file, 
        "/lib/entry.o", 
        source_file, 
        "/lib/oslib.o", 
        NULL
    };

    int pid = spawn("/path/tcc.elf", tcc_args, NULL);
    if (pid < 0) {
        printf("Failed to execute TCC!\n");
        return 1;
    }

    waitpid(pid);
    
    if (get_file_size(out_file) > 0) {
        set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
        printf("Build successful! Run with: ./%s\n", out_file);
        set_color(COLOR_WHITE, COLOR_BLACK);
    } else {
        printf("Build failed.\n");
    }
    return 0;
}