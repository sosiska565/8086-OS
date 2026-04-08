#include <oslib.h>

void print_usage() {
    set_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
    printf("eza - File System Utility\n");
    set_color(COLOR_LIGHT_GRAY, COLOR_BLACK);
    printf("Usage:\n");
    printf("  eza mkfile <name> [text] - Create a file\n");
    printf("  eza mkdir <name>         - Create a directory\n");
    printf("  eza rm <name>            - Remove file/directory recursively\n");
    printf("  eza cp <src> <dst>       - Copy file/directory recursively\n");
    printf("  eza mv <src> <dst>       - Move file/directory\n");
}

void rm_recursive(char* path) {
    vfs_dirent_t ent;
    
    while (readdir(path, 0, &ent) == 1) {
        char sub[256]; sprintf(sub, "%s/%s", path, ent.name);
        if (ent.type == VFS_ATTR_DIR) rm_recursive(sub);
        else delete_file(sub);
    }
    delete_file(path);
}

void cp_recursive(char* src, char* dst) {
    mkdir(dst);
    vfs_dirent_t ent; int idx = 0;
    while (readdir(src, idx++, &ent) == 1) {
        char s_sub[256]; sprintf(s_sub, "%s/%s", src, ent.name);
        char d_sub[256]; sprintf(d_sub, "%s/%s", dst, ent.name);
        
        if (ent.type == VFS_ATTR_DIR) {
            cp_recursive(s_sub, d_sub);
        } else {
            int sz = get_file_size(s_sub);
            uint8_t* buf = malloc(sz + 1);
            read_file(s_sub, buf);
            write_file(d_sub, buf, sz);
            free(buf);
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 2) { print_usage(); return 1; }
    char* cmd = argv[1];

    if (strcmp(cmd, "mkfile") == 0 || strcmp(cmd, "touch") == 0) {
        if (argc < 3) return 1;
        char buf[512]; buf[0] = '\0';
        if (argc > 3) {
            for (int i=3; i<argc; i++) { strcat(buf, argv[i]); if (i < argc - 1) strcat(buf, " "); }
        }
        write_file(argv[2], (uint8_t*)buf, strlen(buf));
    }
    else if (strcmp(cmd, "mkdir") == 0) {
        if (argc < 3) return 1;
        mkdir(argv[2]);
    }
    else if (strcmp(cmd, "rm") == 0) {
        if (argc < 3) return 1;
        int sz = get_file_size(argv[2]);
        if (sz >= 0) delete_file(argv[2]); 
        else rm_recursive(argv[2]);        
    }
    else if (strcmp(cmd, "cp") == 0) {
        if (argc < 4) return 1;
        int sz = get_file_size(argv[2]);
        if (sz >= 0) {
            uint8_t* buf = malloc(sz);
            read_file(argv[2], buf);
            write_file(argv[3], buf, sz);
            free(buf);
        } else {
            cp_recursive(argv[2], argv[3]);
        }
    }
    else if (strcmp(cmd, "mv") == 0) {
        if (argc < 4) return 1;
        int sz = get_file_size(argv[2]);
        if (sz >= 0) {
            uint8_t* buf = malloc(sz);
            read_file(argv[2], buf);
            write_file(argv[3], buf, sz);
            free(buf);
            delete_file(argv[2]);
        } else {
            cp_recursive(argv[2], argv[3]);
            rm_recursive(argv[2]);
        }
    }
    return 0;
}