#include <oslib.h>

int main(int argc, char** argv) {
    if(argc < 2) { printf("usage: sudo <command>\n"); return 1; }
    
    if (getuid() != 0) {
        int uid; uint32_t hash;
        get_user_info("root", &uid, &hash);
        
        printf("[sudo] password for root: ");
        char pass[32]; getpass(pass, 31);
        if (hash_pw(pass) != hash) {
            set_color(4, 0); printf("sudo: 1 incorrect password attempt\n"); set_color(15, 0);
            return 1;
        }
    }

    setuid(0); 
    
    
    char resolved[256];
    int found = 0;
    for(int i=0; argv[1][i]; i++) if (argv[1][i] == '/') { strcpy(resolved, argv[1]); found=1; break; }
    
    if(!found) {
        char path[64]; getenv("PATH", path);
        sprintf(resolved, "%s/%s.elf", path, argv[1]);
    }
    
    int pid = spawn(resolved, &argv[1], NULL);
    if(pid > 0) waitpid(pid);
    else printf("sudo: command not found\n");
    
    return 0;
}