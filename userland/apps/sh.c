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

void to_lower_str(char* str) {
    for(int i = 0; str[i]; i++) {
        if(str[i] >= 'A' && str[i] <= 'Z') str[i] += 32;
    }
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
            to_lower_str(entry.name); 
            if (path_cmd_count < MAX_BINARIES) {
            int len = strlen(entry.name);
            if (len > 4 && strcmp(entry.name + len - 4, ".elf") == 0) {
                for(int i = 0; i < len - 4; i++) path_cmds[path_cmd_count][i] = entry.name[i];
                    path_cmds[path_cmd_count][len - 4] = '\0';    
                } else {
                    strcpy(path_cmds[path_cmd_count], entry.name);
                }
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

int smart_readline(char* buffer) {
    int pos = 0; 
    int len = 0; 
    int hist_idx = history_count;
    int max_drawn_len = 0;
    buffer[0] = '\0';
    
    int prompt_x, prompt_y; 
    get_cursor(&prompt_x, &prompt_y);

    while(1) {
        
        set_cursor(prompt_x, prompt_y);
        for(int i = 0; i < max_drawn_len + 2; i++) print_char(' ');
        
        
        set_cursor(prompt_x, prompt_y);
        set_color(c_char, 0);
        printf("%s", buffer);
        
        
        char* sug = get_suggestion(buffer);
        int sug_len = 0;
        if (sug && pos == len && len > 0 && strlen(sug) > len) {
            sug_len = strlen(sug) - len;
            set_color(8, 0); 
            printf("%s", sug + len);
        }
        
        max_drawn_len = len > (len + sug_len) ? len : (len + sug_len);

        
        set_cursor(prompt_x + pos, prompt_y);
        
        
        set_color(0, 15); 
        print_char(buffer[pos] == '\0' ? ' ' : buffer[pos]);
        set_cursor(prompt_x + pos, prompt_y); 

        
        char c = getc();

        
        set_cursor(prompt_x + pos, prompt_y);
        set_color(c_char, 0);
        print_char(buffer[pos] == '\0' ? ' ' : buffer[pos]); 

        if (c == '\n' || c == '\r') { 
            set_cursor(prompt_x + len, prompt_y);
            print_char('\n'); 
            break; 
        } 
        else if (c == KEY_CTRL_L) { return 1; } 
        else if (c == KEY_UP) { 
            if (hist_idx > 0) {
                hist_idx--; strcpy(buffer, history[hist_idx]); 
                len = strlen(buffer); pos = len;
            } 
        } 
        else if (c == KEY_DOWN) { 
            if (hist_idx < history_count) {
                hist_idx++;
                if (hist_idx == history_count) { buffer[0] = '\0'; len = 0; pos = 0; } 
                else { strcpy(buffer, history[hist_idx]); len = strlen(buffer); pos = len; }
            } 
        }
        else if (c == KEY_LEFT) {
            if (pos > 0) pos--;
        }
        else if (c == KEY_RIGHT) {
            if (pos < len) pos++;
        }
        else if (c == '\b') { 
            if (pos > 0) {
                
                for (int i = pos; i <= len; i++) buffer[i - 1] = buffer[i];
                pos--; len--;
            } 
        } 
        else if (c == '\t') { 
            if (sug && pos == len) { strcpy(buffer, sug); len = strlen(buffer); pos = len; }
        } 
        else if (c >= 32 && c <= 126 && len < 254) { 
            
            for (int i = len; i >= pos; i--) buffer[i + 1] = buffer[i];
            buffer[pos] = c;
            pos++; len++;
        }
    }
    return 0;
}

void split_args(char* input, char* argv[], int* argc) {
    *argc = 0;
    char* p = input;
    
    while (*p) {
        while (*p == ' ') p++; 
        if (!*p) break;
        
        if (*p == '>') {
            argv[(*argc)++] = ">";
            *p = '\0';
            p++;
            continue;
        }
        
        argv[(*argc)++] = p;
        while (*p && *p != ' ' && *p != '>') p++;
        
        if (*p == '>') {
            *p = '\0'; 
        } else if (*p == ' ') {
            *p = '\0';
            p++;
        }
    }
    argv[*argc] = NULL;
}

int resolve_path(char* cmd, char* resolved) {
    for(int i=0; cmd[i]; i++) if (cmd[i] == '/') { strcpy(resolved, cmd); return 1; }
    char full_path[256];
    
    
    sprintf(full_path, "%s/%s", env_path, cmd);
    if (get_file_size(full_path) > 0) { strcpy(resolved, full_path); return 1; }
    
    
    sprintf(full_path, "%s/%s.elf", env_path, cmd);
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
            printf("Included Apps  : ls, cat, nevim, eza, fcc, echo\n");
        }
        else if (strcmp(cmd, "cd") == 0) { if (arg_count > 1) { if (chdir(args[1]) != 0) printf("cd: No such directory\n"); } }
        else if (strcmp(cmd, "pwd") == 0) { char c[256]; getcwd(c); printf("%s\n", c); }
        else if (strcmp(cmd, "clear") == 0) { clear_screen(); }
        else if (strcmp(cmd, "rehash") == 0) { load_config(); rehash_path(); printf("Config reloaded & Path refreshed.\n"); }
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