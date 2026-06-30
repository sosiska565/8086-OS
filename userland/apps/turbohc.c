#include <oslib.h>

#define MAX_LINES 1000
#define MAX_LINE_LEN 128
#define MAX_FILES 100


int term_w, term_h;
int active_pane = 1; 

char editor[MAX_LINES][MAX_LINE_LEN];
int num_lines = 1;
int cx = 0, cy = 0;       
int offset_x = 0, offset_y = 0; 
char current_file[256] = ""; 
int is_modified = 0;

vfs_dirent_t files[MAX_FILES];
int num_files = 0;
int exp_sel = 0;
int exp_offset = 0;
char cwd[256] = "/"; 


int r_fg = COLOR_WHITE;
int r_bg = COLOR_BLUE;
char r_buf[512];
int r_bi = 0;

int isalpha(int c) {
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

int isdigit(int c) {
    return (c >= '0' && c <= '9');
}

int isalnum(int c) {
    return (isalpha(c) || isdigit(c));
}

void r_flush() {
    if (r_bi > 0) {
        r_buf[r_bi] = '\0';
        set_color(r_fg, r_bg);
        printf("%s", r_buf); 
        r_bi = 0;
    }
}

void r_putc(char c, int fg, int bg) {
    if (r_fg != fg || r_bg != bg || r_bi >= 500) {
        r_flush();
        r_fg = fg; r_bg = bg;
    }
    r_buf[r_bi++] = c;
}


void force_redraw_all() {
    set_color(COLOR_WHITE, COLOR_BLUE);
    for(int i = 0; i < term_h; i++) {
        set_cursor(0, i);
        
        
        int w = (i == term_h - 1) ? term_w - 1 : term_w;
        for(int j = 0; j < w; j++) print_char(' ');
    }
}


int is_keyword(char *w) {
    if (!strcmp(w, "int") || !strcmp(w, "void") || !strcmp(w, "char") || 
        !strcmp(w, "if") || !strcmp(w, "else") || !strcmp(w, "while") || 
        !strcmp(w, "return") || !strcmp(w, "struct") || !strcmp(w, "include")) return 1;
    return 0;
}

void adjust_view() {
    int ed_w = term_w - 22;
    int ed_h = term_h - 2;
    if (cy < offset_y) offset_y = cy;
    if (cy >= offset_y + ed_h) offset_y = cy - ed_h + 1;
    if (cx < offset_x) offset_x = cx;
    if (cx >= offset_x + ed_w) offset_x = cx - ed_w + 1;
}


void load_dir() {
    num_files = 0; vfs_dirent_t entry; int idx = 0;
    if (strcmp(cwd, "/") != 0) {
        strcpy(files[num_files].name, "..");
        files[num_files].type = VFS_ATTR_DIR;
        num_files++;
    }
    while (readdir(cwd, idx++, &entry) == 1 && num_files < MAX_FILES) {
        files[num_files++] = entry;
    }
    if (exp_sel >= num_files) exp_sel = 0;
}

void load_file(char* filename) {
    int sz = get_file_size(filename);
    if (sz <= 0) {
        strcpy(current_file, filename);
        num_lines = 1; editor[0][0] = '\0';
        cx = 0; cy = 0; is_modified = 0; return;
    }
    uint8_t* buf = malloc(sz + 1);
    read_file(filename, buf); buf[sz] = '\0';

    num_lines = 0; int line_idx = 0;
    for (int i = 0; i < sz; i++) {
        if (buf[i] == '\n') {
            editor[num_lines][line_idx] = '\0';
            num_lines++; line_idx = 0;
            if (num_lines >= MAX_LINES) break;
        } else if (buf[i] != '\r' && line_idx < MAX_LINE_LEN - 1) {
            editor[num_lines][line_idx++] = buf[i];
        }
    }
    if (num_lines < MAX_LINES) { editor[num_lines][line_idx] = '\0'; num_lines++; }
    free(buf); strcpy(current_file, filename);
    cx = 0; cy = 0; offset_y = 0; offset_x = 0; is_modified = 0;
}

void save_file() {
    if (current_file[0] == '\0') return;
    int total_sz = 0;
    for (int i = 0; i < num_lines; i++) total_sz += strlen(editor[i]) + 1; 

    uint8_t* buf = malloc(total_sz + 1);
    int pos = 0;
    for (int i = 0; i < num_lines; i++) {
        int l = strlen(editor[i]);
        memcpy(buf + pos, editor[i], l);
        pos += l;
        buf[pos++] = '\n';
    }
    buf[pos] = '\0'; 
    write_file(current_file, buf, pos);
    free(buf); is_modified = 0;
}


void ed_insert_char(char c) {
    int len = strlen(editor[cy]);
    if (len >= MAX_LINE_LEN - 1) return;
    memmove(&editor[cy][cx + 1], &editor[cy][cx], len - cx + 1);
    editor[cy][cx] = c; cx++; is_modified = 1;
    adjust_view();
}

void ed_newline() {
    if (num_lines >= MAX_LINES) return;
    for (int i = num_lines; i > cy + 1; i--) strcpy(editor[i], editor[i - 1]);
    strcpy(editor[cy + 1], &editor[cy][cx]);
    editor[cy][cx] = '\0';
    cy++; cx = 0; num_lines++; is_modified = 1;
    adjust_view();
}

void ed_backspace() {
    if (cx > 0) {
        int len = strlen(editor[cy]);
        memmove(&editor[cy][cx - 1], &editor[cy][cx], len - cx + 1);
        cx--; is_modified = 1;
        adjust_view();
    } else if (cy > 0) {
        int prev_len = strlen(editor[cy - 1]);
        if (prev_len + strlen(editor[cy]) < MAX_LINE_LEN) {
            strcat(editor[cy - 1], editor[cy]);
            for (int i = cy; i < num_lines - 1; i++) strcpy(editor[i], editor[i + 1]);
            num_lines--; cy--; cx = prev_len; is_modified = 1;
            adjust_view();
        }
    }
}


void draw_ui() {
    
    set_cursor(0, 0);
    set_color(COLOR_BLACK, COLOR_LIGHT_GRAY);
    char buf[256];
    for(int i=0; i<term_w; i++) buf[i] = ' '; buf[term_w] = '\0';
    printf("%s", buf); 
    
    set_cursor(2, 0);
    printf(" TurboHC v1.5 | HardCore IDE ");

    
    set_cursor(0, term_h - 1);
    set_color(COLOR_BLACK, COLOR_LIGHT_GRAY);
    for(int i=0; i<term_w - 1; i++) buf[i] = ' '; buf[term_w - 1] = '\0'; 
    printf("%s", buf);
    
    set_cursor(1, term_h - 1);
    char status[128];
    sprintf(status, "F1:Save F2:Comp ESC:Pane/Quit | %s %s", 
        current_file[0] ? current_file : "Untitled", is_modified ? "[*]" : "");
    if(strlen(status) > term_w - 2) status[term_w - 2] = '\0';
    printf("%s", status);

    
    for (int i = 1; i < term_h - 1; i++) {
        set_cursor(20, i);
        set_color(COLOR_LIGHT_GRAY, COLOR_BLUE);
        print_char('|');
    }
}

void draw_explorer() {
    int exp_w = 20;
    int max_disp = term_h - 2;

    for (int i = 0; i < max_disp; i++) {
        set_cursor(0, i + 1);
        int fidx = exp_offset + i;
        
        int bg = COLOR_BLUE;
        if (active_pane == 0 && fidx == exp_sel) bg = COLOR_CYAN;
        
        if (fidx < num_files) {
            int fg = (files[fidx].type == VFS_ATTR_DIR) ? COLOR_YELLOW : COLOR_WHITE;
            if (bg == COLOR_CYAN) fg = COLOR_BLACK; 
            
            char buf[32];
            sprintf(buf, "%s%s", files[fidx].type == VFS_ATTR_DIR ? "/" : " ", files[fidx].name);
            for(int j=strlen(buf); j<exp_w; j++) buf[j] = ' '; buf[exp_w] = '\0';
            
            set_color(fg, bg);
            printf("%s", buf);
        } else {
            set_color(COLOR_WHITE, COLOR_BLUE);
            char buf[32];
            for(int j=0; j<exp_w; j++) buf[j] = ' '; buf[exp_w] = '\0';
            printf("%s", buf);
        }
    }
}

void draw_editor() {
    int ed_w = term_w - 22; 
    int ed_h = term_h - 2;

    for (int i = 0; i < ed_h; i++) {
        set_cursor(21, i + 1);
        int lidx = offset_y + i;
        
        if (lidx >= num_lines) {
            set_color(COLOR_WHITE, COLOR_BLUE);
            char buf[128]; for(int j=0; j<ed_w; j++) buf[j] = ' '; buf[ed_w] = '\0';
            printf("%s", buf);
            continue;
        }

        char* line = editor[lidx];
        int len = strlen(line);

        
        int colors[MAX_LINE_LEN];
        for(int j=0; j<len; j++) colors[j] = COLOR_WHITE;

        for(int j=0; j<len; ) {
            char c = line[j];
            if (c == '"') {
                colors[j++] = COLOR_LIGHT_GREEN;
                while(j < len && line[j] != '"') { colors[j++] = COLOR_LIGHT_GREEN; }
                if (j < len) { colors[j++] = COLOR_LIGHT_GREEN; }
                continue;
            }
            if (isalpha(c) || c == '_') {
                char word[64]; int wi = 0; int start = j;
                while (j < len && (isalnum(line[j]) || line[j] == '_') && wi < 63) {
                    word[wi++] = line[j++];
                }
                word[wi] = '\0';
                if (is_keyword(word)) {
                    for(int k=start; k<j; k++) colors[k] = COLOR_YELLOW;
                }
                continue;
            }
            if (strchr("{}()[]#<>;=+-*/", c)) colors[j] = COLOR_LIGHT_CYAN;
            else if (isdigit(c)) colors[j] = COLOR_LIGHT_MAGENTA;
            j++;
        }

        int px = offset_x;
        while (px < offset_x + ed_w) {
            int is_cursor = (active_pane == 1 && lidx == cy && px == cx);
            char c = (px < len) ? line[px] : ' ';
            int fg = (px < len) ? colors[px] : COLOR_WHITE;

            if (is_cursor) {
                r_putc(c, COLOR_BLACK, COLOR_LIGHT_GREEN);
            } else {
                r_putc(c, fg, COLOR_BLUE);
            }
            px++;
        }
        r_flush(); 
    }
}


void compile_current() {
    if (current_file[0] == '\0') return;
    save_file();

    set_color(COLOR_WHITE, COLOR_BLACK);
    clear_screen();
    set_cursor(0, 0);
    
    printf("=========================================\n");
    printf(" Compiling: %s\n", current_file);
    printf("=========================================\n\n");

    char* gcc_args[] = {"gcc", current_file, NULL};
    int pid = spawn("/path/gcc.elf", gcc_args, NULL);
    
    if (pid > 0) {
        waitpid(pid);
    } else {
        set_color(COLOR_LIGHT_RED, COLOR_BLACK);
        printf("Error: Could not launch /path/gcc.elf\n");
    }

    set_color(COLOR_YELLOW, COLOR_BLACK);
    printf("\n[ Compilation finished. Press ANY KEY to return to TurboHC ]\n");
    
    while(poll_key() != 0); 
    while(poll_key() == 0) yield(); 

    force_redraw_all();
}

int main(int argc, char** argv) {
    get_term_size(&term_w, &term_h);
    for(int i=0; i<MAX_LINES; i++) editor[i][0] = '\0';

    
    while(poll_key() != 0);

    
    if (argc > 1) {
        strcpy(current_file, argv[1]);
        char* last_slash = strrchr(argv[1], '/');
        if (last_slash) {
            if (last_slash == argv[1]) strcpy(cwd, "/");
            else {
                int dir_len = last_slash - argv[1];
                strncpy(cwd, argv[1], dir_len);
                cwd[dir_len] = '\0';
            }
        } else {
            strcpy(cwd, "/");
        }
        load_file(current_file);
    } else {
        strcpy(cwd, "/");
    }
    
    force_redraw_all();
    load_dir();
    
    while(1) {
        draw_ui();
        draw_explorer();
        draw_editor();
        
        
        uint32_t key_val = 0;
        while ((key_val = poll_key()) == 0) yield();
        
        char sc = key_val & 0xFF; 

        if (sc == KEY_F1) save_file();
        else if (sc == KEY_F2) compile_current();
        else if (sc == KEY_ESC) {
            if (active_pane == 1) active_pane = 0; 
            else break; 
        }
        else if (active_pane == 0) { 
            if (sc == KEY_UP) {
                if (exp_sel > 0) exp_sel--;
                if (exp_sel < exp_offset) exp_offset--;
            } else if (sc == KEY_DOWN) {
                if (exp_sel < num_files - 1) exp_sel++;
                if (exp_sel >= exp_offset + (term_h - 2)) exp_offset++;
            } else if (sc == KEY_ENTER) {
                if (files[exp_sel].type == VFS_ATTR_DIR) {
                    if (strcmp(files[exp_sel].name, "..") == 0) {
                        char* last_slash = strrchr(cwd, '/');
                        if (last_slash && last_slash != cwd) *last_slash = '\0';
                        else strcpy(cwd, "/");
                    } else {
                        if (strcmp(cwd, "/") != 0) strcat(cwd, "/");
                        strcat(cwd, files[exp_sel].name);
                    }
                    exp_sel = 0; exp_offset = 0;
                    load_dir();
                } else {
                    char fullpath[256];
                    if (strcmp(cwd, "/") == 0) sprintf(fullpath, "/%s", files[exp_sel].name);
                    else sprintf(fullpath, "%s/%s", cwd, files[exp_sel].name);
                    load_file(fullpath);
                    active_pane = 1; 
                }
            }
        }
        else if (active_pane == 1) { 
            if (sc == KEY_UP) {
                if (cy > 0) {
                    cy--;
                    if (cx > strlen(editor[cy])) cx = strlen(editor[cy]);
                    adjust_view();
                }
            } else if (sc == KEY_DOWN) {
                if (cy < num_lines - 1) {
                    cy++;
                    if (cx > strlen(editor[cy])) cx = strlen(editor[cy]);
                    adjust_view();
                }
            } else if (sc == KEY_LEFT) {
                if (cx > 0) { cx--; adjust_view(); }
                else if (cy > 0) { cy--; cx = strlen(editor[cy]); adjust_view(); }
            } else if (sc == KEY_RIGHT) {
                if (cx < strlen(editor[cy])) { cx++; adjust_view(); }
                else if (cy < num_lines - 1) { cy++; cx = 0; adjust_view(); }
            } else if (sc == KEY_ENTER) {
                ed_newline();
            } else if (sc == KEY_BACKSPACE) {
                ed_backspace();
            } else if (sc == KEY_TAB) {
                for(int t=0; t<4; t++) ed_insert_char(' ');
            } else if (sc >= 32 && sc <= 126) {
                ed_insert_char(sc);
            }
        }
    }

    set_color(COLOR_LIGHT_GRAY, COLOR_BLACK);
    clear_screen();
    return 0;
}