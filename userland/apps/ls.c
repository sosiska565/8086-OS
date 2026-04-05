#include <oslib.h>

int main(int argc, char** argv) {
    vfs_dirent_t entry; 
    int idx = 0;
    char* target = (argc > 1) ? argv[1] : ".";
    
    while (readdir(target, idx++, &entry) == 1) {
        if (entry.type == VFS_ATTR_DIR) { set_color(9, 0); printf("[DIR]  "); }
        else if (entry.type == VFS_ATTR_DEV) { set_color(14, 0); printf("[DEV]  "); }
        else { set_color(7, 0); printf("[FILE] "); }
        
        printf(entry.name);
        if (entry.type == VFS_ATTR_FILE) { printf("\t"); printf("%d", entry.size); print(" B"); }
        printf("\n");
    }
    set_color(7, 0);
    return 0;
}