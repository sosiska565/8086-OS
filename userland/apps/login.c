#include <oslib.h>

void first_boot_setup() {
    clear_screen();
    set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    printf("\n========================================\n");
    printf("      8086-OS First Boot Setup\n");
    printf("========================================\n\n");
    set_color(COLOR_WHITE, COLOR_BLACK);
    
    printf("Welcome! Since this is the first boot, let's set up your system.\n\n");
    
    
    printf("1. Set a password for the 'root' (Administrator) account.\n");
    printf("   New root password: ");
    char root_pw[32]; getpass(root_pw, 31);
    while (root_pw[0] == '\0') {
        set_color(COLOR_LIGHT_RED, COLOR_BLACK);
        printf("   Root password cannot be empty! Try again.\n");
        set_color(COLOR_WHITE, COLOR_BLACK);
        printf("   New root password: ");
        getpass(root_pw, 31);
    }
    
    
    printf("\n2. Create your primary user account.\n");
    printf("   Username (default: user): ");
    char uname[32]; gets(uname, 31);
    if (uname[0] == '\0') strcpy(uname, "user"); 
    
    printf("   Password for '%s' (leave empty for no password): ", uname);
    char upw[32]; getpass(upw, 31);
    
    printf("\nSaving configuration...\n");
    
    
    update_user("root", 0, hash_pw(root_pw), 0);
    update_user(uname, 1000, upw[0] ? hash_pw(upw) : 0, 0);
    
    set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    printf("Setup complete! Press ENTER to proceed to the login screen.\n");
    set_color(COLOR_WHITE, COLOR_BLACK);
    char dummy[2]; gets(dummy, 1);
}

int main() {
    
    if (get_file_size("/users.cfg") <= 0) {
        first_boot_setup();
    }

    
    while(1) {
        setuid(0); 
        clear_screen();
        
        set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
        printf("\n\n=== 8086-OS Login ===\n");
        set_color(COLOR_WHITE, COLOR_BLACK);
        
        printf("Username: ");
        char user[32]; gets(user, 31);
        
        if (user[0] == '\0') continue; 
        
        int uid; uint32_t hash;
        if (get_user_info(user, &uid, &hash)) {
            if (hash != 0) { 
                printf("Password: ");
                char pass[32]; getpass(pass, 31);
                
                if (hash_pw(pass) == hash) {
                    setuid(uid);
                    printf("Welcome, %s!\n", user);
                    int pid = spawn("/path/sh.elf", NULL, NULL);
                    waitpid(pid);
                } else {
                    set_color(COLOR_LIGHT_RED, COLOR_BLACK); 
                    printf("\nLogin incorrect.\n"); 
                    set_color(COLOR_WHITE, COLOR_BLACK);
                    for(volatile int i=0; i<10000000; i++); 
                }
            } else { 
                setuid(uid);
                printf("Welcome, %s!\n", user);
                int pid = spawn("/path/sh.elf", NULL, NULL);
                waitpid(pid);
            }
        } else {
            set_color(COLOR_LIGHT_RED, COLOR_BLACK); 
            printf("\nLogin incorrect.\n"); 
            set_color(COLOR_WHITE, COLOR_BLACK);
            for(volatile int i=0; i<10000000; i++);
        }
    }
    return 0;
}