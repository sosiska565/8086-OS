/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/apps/sh.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include <oslib.h>
#include <signal.h>

#define MAX_HISTORY 16
char history[MAX_HISTORY][256];
int history_count = 0;

char env_path[256] = "/path";
char path_sug_buf[128];
int c_user = 11, c_host = 10, c_path = 9, c_char = 15;

#define MAX_BINARIES 64
char path_cmds[MAX_BINARIES][32];
int path_cmd_count = 0;

const char* builtins[] = { "help", "cd", "pwd", "exit", "clear", "rehash", "whoiam", "su", "mount", "umount", NULL };

int starts_with(const char* str, const char* prefix) {
    while (*prefix) { if (*prefix++ != *str++) return 0; } return 1;
}

void to_lower_str(char* str) {
    for(int i = 0; str[i]; i++) {
        if(str[i] >= 'A' && str[i] <= 'Z') str[i] += 32;
    }
}

void load_config() {
    char *val;
    if ((val = getenv("PATH"))) strcpy(env_path, val);
    if ((val = getenv("PROMPT_USER_COLOR"))) c_user = atoi(val);
    if ((val = getenv("PROMPT_HOST_COLOR"))) c_host = atoi(val);
    if ((val = getenv("PROMPT_PATH_COLOR"))) c_path = atoi(val);
    if ((val = getenv("PROMPT_CHAR_COLOR"))) c_char = atoi(val);
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

char* get_path_suggestion(char* buffer) {
    int len = strlen(buffer);
    if (len == 0) return NULL;
    
    int word_start = len - 1;
    while (word_start >= 0 && buffer[word_start] != ' ') word_start--;
    word_start++; 

    char* target = &buffer[word_start];
    if (strlen(target) == 0) return NULL; 

    char dir_path[128] = "."; 
    char file_prefix[64] = "";
    
    char* last_slash = strrchr(target, '/');
    if (last_slash) {
        int dir_len = last_slash - target + 1;
        strncpy(dir_path, target, dir_len);
        dir_path[dir_len] = '\0';
        strcpy(file_prefix, last_slash + 1);
    } else {
        strcpy(file_prefix, target);
    }

    
    char file_prefix_upper[64];
    strcpy(file_prefix_upper, file_prefix);
    to_upper(file_prefix_upper); 
    

    vfs_dirent_t ent;
    int idx = 0;
    int matches = 0;
    char last_match[64] = "";
    uint8_t match_type = 0;

    while (readdir(dir_path, idx++, &ent) == 1) {
        
        char ent_name_upper[64];
        strncpy(ent_name_upper, ent.name, sizeof(ent_name_upper) - 1);
        ent_name_upper[sizeof(ent_name_upper) - 1] = '\0';
        to_upper(ent_name_upper); 
        
        if (strncmp(ent_name_upper, file_prefix_upper, strlen(file_prefix_upper)) == 0) {
            strcpy(last_match, ent.name);
            match_type = ent.type;
            matches++;
        }
    }

    if (matches == 1) {
        char* remainder = last_match + strlen(file_prefix);
        strcpy(path_sug_buf, remainder);
        
        if (match_type == VFS_ATTR_DIR) {
            strcat(path_sug_buf, "/");
        }
        return to_lower(path_sug_buf);
    }
    
    return NULL; 
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
        
        char* cmd_sug = get_suggestion(buffer);
        char* path_sug = NULL;
        int sug_len = 0;

        
        if (cmd_sug && pos == len && len > 0 && strlen(cmd_sug) > len) {
            sug_len = strlen(cmd_sug) - len;
            set_color(8, 0); 
            printf("%s", cmd_sug + len);
        } 
        
        else if (!cmd_sug && pos == len && len > 0) {
            char tmpbuff[256];
            strcpy(tmpbuff, buffer);
            to_upper(tmpbuff);

            path_sug = get_path_suggestion(tmpbuff);
            if (path_sug) {
                sug_len = strlen(path_sug);
                set_color(8, 0); 
                printf("%s", path_sug);
            }
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

        if (c == '\n' || c == '\r') { set_cursor(prompt_x + len, prompt_y); print_char('\n'); break; } 
        else if (c == KEY_CTRL_L) { return 1; } 
        else if (c == KEY_UP) { if (hist_idx > 0) { hist_idx--; strcpy(buffer, history[hist_idx]); len = strlen(buffer); pos = len; } } 
        else if (c == KEY_DOWN) { if (hist_idx < history_count) { hist_idx++; if (hist_idx == history_count) { buffer[0] = '\0'; len = 0; pos = 0; } else { strcpy(buffer, history[hist_idx]); len = strlen(buffer); pos = len; } } }
        else if (c == KEY_LEFT) { if (pos > 0) pos--; }
        else if (c == KEY_RIGHT) { if (pos < len) pos++; }
        else if (c == '\b') { if (pos > 0) { for (int i = pos; i <= len; i++) buffer[i - 1] = buffer[i]; pos--; len--; } } 
        else if (c == '\t') { 
            if (cmd_sug && pos == len) { 
                strcpy(buffer, cmd_sug); 
                len = strlen(buffer); 
                pos = len; 
            } else if (path_sug && pos == len) {
                strcat(buffer, path_sug);
                len = strlen(buffer); 
                pos = len; 
            }
        } 
        else if (c >= 32 && c <= 126 && len < 254) { for (int i = len; i >= pos; i--) buffer[i + 1] = buffer[i]; buffer[pos] = c; pos++; len++; }
    }
    return 0;
}

void split_args(char* input, char* argv[], int* argc) {
    *argc = 0;
    char* p = input;
    while (*p) {
        while (*p == ' ') p++; 
        if (!*p) break;
        if (*p == '>') { argv[(*argc)++] = ">"; *p = '\0'; p++; continue; }
        if (*p == '|') { argv[(*argc)++] = "|"; *p = '\0'; p++; continue; }
        if (*p == '"') { p++; argv[(*argc)++] = p; while (*p && *p != '"') p++; if (*p == '"') { *p = '\0'; p++; } } 
        else { 
            argv[(*argc)++] = p; 
            while (*p && *p != ' ' && *p != '>' && *p != '|') p++; 
            if (*p == '>' || *p == '|') { 
                char tmp = *p; *p = '\0'; p++; 
                if(tmp=='>') argv[(*argc)++] = ">"; else argv[(*argc)++] = "|"; 
            } else if (*p == ' ') { 
                *p = '\0'; p++; 
            } 
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
    char cwd[256]; getcwd(cwd, 256);
    set_color(12, 0); printf("\n┌──(root@8086-os)─[");
    set_color(9, 0); printf("%s", cwd);
    set_color(12, 0); printf("]\n└─# ");
    set_color(7, 0);
}

int main(int argc, char** argv) {
    char input[256]; char* args[32]; int arg_count;
    load_config(); rehash_path();
    clear_screen();

    signal(SIGINT, SIG_IGN);

    while (1) {
        draw_prompt();
        if (smart_readline(input) == 1) { clear_screen(); continue; }

        if (input[0] == '\0') continue;
        add_history(input);
        split_args(input, args, &arg_count);
        char* cmd = args[0];

        int run_in_background = 0;
        if (arg_count > 0 && strcmp(args[arg_count - 1], "&") == 0) {
            run_in_background = 1;
            args[arg_count - 1] = NULL; 
            arg_count--;
        }

        char* redirect_file = NULL;
        int pipe_idx = -1;
        
        for(int i=0; i<arg_count; i++) {
            if (strcmp(args[i], ">") == 0) { args[i] = NULL; if (i + 1 < arg_count) redirect_file = args[i+1]; break; }
            if (strcmp(args[i], "|") == 0) { args[i] = NULL; pipe_idx = i + 1; break; }
        }

        if (strcmp(cmd, "help") == 0) {
            set_color(7, 0);
            printf("Shell built-ins: cd, pwd, exit, clear, rehash, help, su, mount, umount\n");
            printf("Included Apps  : ls, cat, nevim, eza, gcc, echo, installer\n");
        }
        else if (strcmp(cmd, "cd") == 0) { if (arg_count > 1) { if (chdir(args[1]) != 0) printf("cd: No such directory\n"); } }
        else if (strcmp(cmd, "pwd") == 0) { char c[256]; getcwd(c, 256); printf("%s\n", c); }
        else if (strcmp(cmd, "clear") == 0) { clear_screen(); }
        else if (strcmp(cmd, "mount") == 0) {
            if (arg_count < 4) printf("Usage: mount /dev/sdX /mnt/folder fs_type\n");
            else {
                int res = mount(args[1], args[2], args[3]);
                if (res == 0) printf("Mounted %s to %s\n", args[1], args[2]);
                else printf("Mount failed with code %d\n", res);
            }
        }
        else if (strcmp(cmd, "umount") == 0) {
            if (arg_count < 2) printf("Usage: umount /mnt/folder\n");
            else {
                if (unmount(args[1]) == 0) printf("Unmounted successfully.\n");
                else printf("Unmount failed.\n");
            }
        }
        else if (strcmp(cmd, "rehash") == 0) { load_config(); rehash_path(); printf("Config reloaded & Path refreshed.\n"); }
        else if (strcmp(cmd, "exit") == 0) { break; } 
        else {
            char resolved[256];
            if (resolve_path(cmd, resolved)) {
                if (pipe_idx != -1) {
                    char resolved2[256];
                    if (resolve_path(args[pipe_idx], resolved2)) {
                        int pfd[2];
                        pipe(pfd);
                        
                        int pid1 = spawn_ext(resolved, args, -1, pfd[1]);
                        int pid2 = spawn_ext(resolved2, &args[pipe_idx], pfd[0], -1);
                        
                        close(pfd[0]);
                        close(pfd[1]);
                        
                        waitpid(pid1); waitpid(pid2);
                    } else printf("sh: right command not found\n");
                } else if (redirect_file) {
                    int fd = open(redirect_file, O_WRONLY | O_CREAT | O_TRUNC);
                    if(fd != -1) {
                        int pid = spawn_ext(resolved, args, -1, fd);
                        close(fd);
                        if (!run_in_background) waitpid(pid);
                        else printf("[%d] running in background\n", pid);
                    } else printf("sh: failed to open redirect file\n");
                } else {
                    int pid = spawn_ext(resolved, args, -1, -1);
                    if (!run_in_background) waitpid(pid);
                    else printf("[%d] running in background\n", pid);
                }
            } else printf("sh: command not found\n");
        }
    }
    return 0;
}
