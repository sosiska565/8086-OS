#include <oslib.h>

int main(int argc, char** argv) {
    if (getuid() != 0) { printf("usermod: Permission denied. Are you root?\n"); return 1; }
    if (argc < 3) {
        printf("Usage:\n  usermod add <name> <uid>\n  usermod passwd <name>\n  usermod rm <name>\n");
        return 1;
    }
    char* op = argv[1]; char* name = argv[2];
    
    if (strcmp(op, "add") == 0) {
        if(argc < 4) { printf("Missing UID\n"); return 1; }
        printf("New password (leave empty for none): ");
        char pass[32]; getpass(pass, 31);
        uint32_t h = (pass[0] == '\0') ? 0 : hash_pw(pass);
        update_user(name, atoi(argv[3]), h, 0);
        printf("User '%s' created.\n", name);
    }
    else if (strcmp(op, "passwd") == 0) {
        int uid; uint32_t h;
        if(!get_user_info(name, &uid, &h)) { printf("User not found.\n"); return 1; }
        printf("New password (leave empty for none): ");
        char pass[32]; getpass(pass, 31);
        h = (pass[0] == '\0') ? 0 : hash_pw(pass);
        update_user(name, uid, h, 0);
        printf("Password updated.\n");
    }
    else if (strcmp(op, "rm") == 0) {
        update_user(name, 0, 0, 1);
        printf("User '%s' removed.\n", name);
    }
    return 0;
}