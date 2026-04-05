#include <oslib.h>

#define MAX_HISTORY 16
char history[MAX_HISTORY][256];
int history_count = 0;

char env_path[256] = "/path";
int c_user = 11, c_host = 10, c_path = 9, c_char = 15;

#define MAX_BINARIES 64
char path_cmds[MAX_BINARIES][32];
int path_cmd_count = 0;

const char* builtins[] = { "help", "cd", "pwd", "exit", "clear", "rehash", "whoiam", "su", NULL };

int starts_with(const char* str, const char* prefix) {
    while (*prefix) { if (*prefix++ != *str++) return 0; } return 1;
}

void load_config() {
    char buf[64];
    if (getenv("PATH", buf)) strcpy(env_path, buf);
    if (getenv("PROMPT_USER_COLOR", buf)) c_user = atoi(buf);
    if (getenv("PROMPT_HOST_COLOR", buf)) c_host = atoi(buf);
    if (getenv("PROMPT_PATH_COLOR", buf)) c_path = atoi(buf);
    if (getenv("PROMPT_CHAR_COLOR", buf)) c_char = atoi(buf);
}

void rehash_path() {
    path_cmd_count = 0; vfs_dirent_t entry; int idx = 0;
    while (readdir(env_path, idx++, &entry) == 1) {
        if (entry.type == VFS_ATTR_FILE) {
            int len = strlen(entry.name);
            if (len > 4 && strcmp(entry.name + len - 4, ".bin") == 0 && path_cmd_count < MAX_BINARIES) {
                for(int i = 0; i < len - 4; i++) path_cmds[path_cmd_count][i] = entry.name[i];
                path_cmds[path_cmd_count][len - 4] = '\0';
                path_cmd_count++;
            }
        }
    }
}

char* get_suggestion(char* input) {
    if (input[0] == '\0') return NULL;
    for (int i = 0; builtins[i] != NULL; i++) if (starts_with(builtins[i], input)) return (char*)builtins[i];
    for (int i = 0; i < path_cmd_count; i++) if (starts_with(path_cmds[i], input)) return path_cmds[i];
    return NULL;
}

void add_history(char* cmd) {
    if(cmd[0] == '\0') return;
    if(history_count > 0 && strcmp(history[history_count-1], cmd) == 0) return;
    if(history_count < MAX_HISTORY) { strcpy(history[history_count++], cmd); } 
    else { for(int i = 1; i < MAX_HISTORY; i++) strcpy(history[i-1], history[i]); strcpy(history[MAX_HISTORY-1], cmd); }
}

void clear_input_line(int prompt_x, int prompt_y, int text_len, int sug_len) {
    set_cursor(prompt_x, prompt_y);
    for(int i = 0; i < text_len + sug_len; i++) print_char(' ');
    set_cursor(prompt_x, prompt_y);
}

int smart_readline(char* buffer) {
    int pos = 0; int last_sug_len = 0; int hist_idx = history_count;
    buffer[0] = '\0';
    int prompt_x, prompt_y; get_cursor(&prompt_x, &prompt_y);
    set_color(c_char, 0); 

    while(1) {
        char c = getc();
        int cx, cy; get_cursor(&cx, &cy);
        for(int i = 0; i < last_sug_len; i++) print_char(' ');
        set_cursor(cx, cy);

        if (c == '\n' || c == '\r') { print_char('\n'); break; } 
        else if (c == 12) { return 1; } 
        else if (c == 17) { 
            if (hist_idx > 0) {
                clear_input_line(prompt_x, prompt_y, pos, last_sug_len);
                hist_idx--; strcpy(buffer, history[hist_idx]); pos = strlen(buffer);
                set_color(c_char, 0); printf("%s", buffer); last_sug_len = 0;
            } continue;
        } 
        else if (c == 18) { 
            if (hist_idx < history_count) {
                clear_input_line(prompt_x, prompt_y, pos, last_sug_len);
                hist_idx++;
                if (hist_idx == history_count) { buffer[0] = '\0'; pos = 0; } 
                else { strcpy(buffer, history[hist_idx]); pos = strlen(buffer); set_color(c_char, 0); printf("%s", buffer); }
                last_sug_len = 0;
            } continue;
        }
        else if (c == '\b') { if (pos > 0) { printf("\b \b"); pos--; buffer[pos] = '\0'; } } 
        else if (c == '\t') { 
            char* sug = get_suggestion(buffer);
            if (sug) { set_color(c_char, 0); printf("%s", sug + pos); strcpy(buffer, sug); pos = strlen(buffer); }
        } 
        else if (c >= 32 && c <= 126 && pos < 254) {
            set_color(c_char, 0); print_char(c); buffer[pos++] = c; buffer[pos] = '\0';
        }

        get_cursor(&cx, &cy);
        char* sug = get_suggestion(buffer);
        if (sug && pos > 0 && strlen(sug) > pos) {
            last_sug_len = strlen(sug) - pos;
            set_color(8, 0); printf("%s", sug + pos); set_cursor(cx, cy); 
        } else last_sug_len = 0;
        set_color(c_char, 0); 
    }
    return 0;
}

void split_args(char* input, char* argv[], int* argc) {
    *argc = 0; int in_word = 0;
    while (*input) {
        if (*input == ' ') { *input = '\0'; in_word = 0; } 
        else if (!in_word) { argv[(*argc)++] = input; in_word = 1; }
        input++;
    }
    argv[*argc] = NULL;
}

int resolve_path(char* cmd, char* resolved) {
    for(int i=0; cmd[i]; i++) if (cmd[i] == '/') { strcpy(resolved, cmd); return 1; }
    char full_path[256];
    sprintf(full_path, "%s/%s.bin", env_path, cmd);
    if (get_file_size(full_path) > 0) { strcpy(resolved, full_path); return 1; }
    return 0;
}

void draw_prompt() {
    char cwd[256]; getcwd(cwd);
    int uid = getuid();

    set_color(c_host, 0); printf("\n┌──(");
    
    if (uid == 0) { set_color(12, 0); printf("root@8086-os"); } 
    else { set_color(c_user, 0); printf("user@8086-os"); }
    
    set_color(c_host, 0); printf(")─[");
    set_color(c_path, 0); printf("%s", cwd);
    set_color(c_host, 0); printf("]\n└─");
    
    if (uid == 0) { set_color(12, 0); printf("# "); } 
    else { set_color(15, 0); printf("$ "); } 
    
    set_color(7, 0);
}

int main(int argc, char** argv) {
    
    setuid(1000);

    char input[256]; char* args[16]; int arg_count;
    load_config(); rehash_path();
    clear_screen();

    while (1) {
        draw_prompt();
        if (smart_readline(input) == 1) { clear_screen(); continue; }

        if (input[0] == '\0') continue;
        add_history(input);
        split_args(input, args, &arg_count);
        char* cmd = args[0];

        
        char* redirect_path = NULL;
        for(int i=0; i<arg_count; i++) {
            if (strcmp(args[i], ">") == 0) {
                args[i] = NULL; 
                if (i + 1 < arg_count) redirect_path = args[i+1];
                break;
            }
        }

        if (strcmp(cmd, "help") == 0) {
            set_color(7, 0);
            printf("Shell built-ins: cd, pwd, exit, clear, rehash, help, su\n");
            printf("Usage: command > file.txt (Output Redirection)\n");
        } 
        else if (strcmp(cmd, "su") == 0) {
            printf("Password: ");
            char pass[32]; gets(pass, 31);
            if (strcmp(pass, "toor") == 0) { 
                setuid(0); printf("Root privileges granted.\n");
            } else { printf("su: Authentication failure\n"); }
        }
        else if (strcmp(cmd, "cd") == 0) { if (arg_count > 1) { if (chdir(args[1]) != 0) printf("cd: No such directory\n"); } }
        else if (strcmp(cmd, "pwd") == 0) { char c[256]; getcwd(c); printf("%s\n", c); }
        else if (strcmp(cmd, "clear") == 0) { clear_screen(); }
        else if (strcmp(cmd, "rehash") == 0) { load_config(); rehash_path(); printf("Config reloaded & Path refreshed.\n"); }
        else if (strcmp(cmd, "whoiam") == 0) {
            printf("PC name: 8086-OS\n");
            printf("username: user\n");
            printf("UID: %d\n", getuid());
        }
        else if (strcmp(cmd, "exit") == 0) { break; } 
        else {
            set_color(7, 0);
            char resolved[256];
            if (resolve_path(cmd, resolved)) {
                
                int pid = spawn(resolved, args, redirect_path);
                if (pid > 0) waitpid(pid);
                else {
                    if (getuid() != 0) printf("sh: Permission denied.\n");
                    else printf("sh: Error executing program.\n");
                }
            } else printf("sh: command not found\n");
        }
    }
    return 0;
}