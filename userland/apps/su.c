#include <oslib.h>

int main(int argc, char** argv) {
    char* target = argc > 1 ? argv[1] : "root";
    int uid; uint32_t hash;
    
    if (!get_user_info(target, &uid, &hash)) {
        printf("su: user %s does not exist\n", target);
        return 1;
    }
    
    
    if (hash != 0 && getuid() != 0) {
        printf("Password: ");
        char pass[32]; getpass(pass, 31);
        if (hash_pw(pass) != hash) {
            set_color(4, 0); printf("su: Authentication failure\n"); set_color(15, 0);
            return 1;
        }
    }
    
    setuid(uid);
    int pid = spawn("/path/sh.elf", NULL, NULL);
    waitpid(pid);
    return 0;
}