#include <oslib.h>

#define MAX_LINE_LEN 256
#define TREE_WIDTH 20

#define PANEL_TREE 0
#define PANEL_ED1  1
#define PANEL_ED2  2
#define PANEL_TERM 3

int active_panel = PANEL_ED1; 
int term_cols = 80; int term_rows = 25;
char current_dir[128] = "/";


int show_tree = 1;
int show_term = 0;
int split_mode = 0; 

typedef struct { int active; int x, y, w, h; } ViewRect;
ViewRect v_tree, v_ed1, v_ed2, v_term;

int is_executable_ext(const char* name) {
    int len = strlen(name);
    if (len >= 4) {
        const char* ext = name + len - 4;
        if (ext[0] == '.' && (ext[1] == 'e' || ext[1] == 'E') && (ext[2] == 'l' || ext[2] == 'L') && (ext[3] == 'f' || ext[3] == 'F')) return 1;
        if (ext[0] == '.' && (ext[1] == 'b' || ext[1] == 'B') && (ext[2] == 'i' || ext[2] == 'I') && (ext[3] == 'n' || ext[3] == 'N')) return 1;
    } return 0;
}

int my_strncmp(const char *s1, const char *s2, int n) {
    while (n > 0 && *s1 && (*s1 == *s2)) { s1++; s2++; n--; }
    if (n == 0) return 0; return (*(unsigned char *)s1 - *(unsigned char *)s2);
}
void my_strncpy(char *dest, const char *src, int n) { int i; for (i = 0; i < n && src[i] != '\0'; i++) dest[i] = src[i]; for ( ; i < n; i++) dest[i] = '\0'; }
void pad_string(char *dest, const char *src, int width) { int len = strlen(src); strcpy(dest, src); for(int i = len; i < width; i++) dest[i] = ' '; dest[width] = '\0'; }
void hex_32_str(uint32_t val, char *buf) { const char *hex = "0123456789ABCDEF"; for(int i=7; i>=0; i--) { buf[i] = hex[val & 0xF]; val >>= 4; } buf[8] = '\0'; }
void hex_8_str(uint8_t val, char *buf) { const char *hex = "0123456789ABCDEF"; buf[0] = hex[(val >> 4) & 0xF]; buf[1] = hex[val & 0xF]; buf[2] = '\0'; }

#define MAX_SUGGESTIONS 16
typedef struct { char name[64]; char signature[64]; int type; char parent[64]; } Symbol; 
Symbol global_symbols[1024]; int symbol_cnt = 0;

typedef struct { char var_name[64]; char type_name[64]; } LocalVar;
LocalVar locals[128]; int local_cnt = 0;

typedef struct { Symbol* matches[MAX_SUGGESTIONS]; int count; int selected; int active; char current_word[64]; int is_dot_context; int offset; } Autocomplete;
Autocomplete ac = {0};

char parsed_files[32][64]; int parsed_count = 0;

const char* base_keywords[] = { "int", "char", "string", "void", "struct", "if", "else", "while", "for", "break", "continue", "return", "syscall", "fn", "let", "mut", "loop", "pub", "impl", NULL };
const char* macros[] = { "#include", "#define", "#ifdef", "#ifndef", "#endif", NULL };

int is_space_c(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
int is_alpha_num_uscore(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '_') || (c == '#'); }

void add_symbol(const char* name, const char* sig, int type, const char* parent) {
    for(int i=0; i<symbol_cnt; i++) { if(strcmp(global_symbols[i].name, name) == 0 && strcmp(global_symbols[i].parent, parent) == 0) return; }
    if (symbol_cnt < 1024) {
        strcpy(global_symbols[symbol_cnt].name, name); strcpy(global_symbols[symbol_cnt].signature, sig);
        global_symbols[symbol_cnt].type = type; strcpy(global_symbols[symbol_cnt].parent, parent); symbol_cnt++;
    }
}

void init_base_symbols() {
    symbol_cnt = 0; local_cnt = 0; parsed_count = 0;
    for(int i=0; base_keywords[i] != NULL; i++) add_symbol(base_keywords[i], "keyword", 0, "");
    for(int i=0; macros[i] != NULL; i++) add_symbol(macros[i], "macro", 3, "");
}

void scan_buffer_for_symbols(char* text, int len) {
    int i = 0; char current_impl[64] = ""; int in_impl = 0;
    while(i < len) {
        
        if (text[i] == '#' && i + 8 < len && my_strncmp(&text[i], "#include", 8) == 0) {
            i += 8; while(i < len && (text[i] == ' ' || text[i] == '<' || text[i] == '"')) i++;
            char inc_name[64]; int k = 0;
            while(i < len && text[i] != '>' && text[i] != '"' && text[i] != '\n' && k < 63) inc_name[k++] = text[i++];
            inc_name[k] = '\0';
            
            int already_parsed = 0;
            for(int p=0; p<parsed_count; p++) if(strcmp(parsed_files[p], inc_name) == 0) already_parsed = 1;
            
            if(!already_parsed && parsed_count < 32) {
                strcpy(parsed_files[parsed_count++], inc_name);
                char inc_path[128];
                sprintf(inc_path, "/include/%s", inc_name);
                int sz = get_file_size(inc_path);
                if (sz <= 0) { sprintf(inc_path, "/lib/%s", inc_name); sz = get_file_size(inc_path); }
                if (sz <= 0) { sprintf(inc_path, "%s/%s", current_dir, inc_name); sz = get_file_size(inc_path); }
                
                if (sz > 0 && sz < 65536) {
                    char* inc_buf = malloc(sz + 1); read_file(inc_path, (uint8_t*)inc_buf); inc_buf[sz] = '\0';
                    scan_buffer_for_symbols(inc_buf, sz); free(inc_buf);
                }
            }
        }
        
        
        if (text[i] == '(') {
            int j = i - 1;
            while(j >= 0 && is_space_c(text[j])) j--;
            if (j >= 0 && is_alpha_num_uscore(text[j])) {
                int end = j;
                while(j >= 0 && is_alpha_num_uscore(text[j])) j--;
                int start = j + 1;
                char func_name[64]; int k = 0;
                for(int m = start; m <= end && k < 63; m++) func_name[k++] = text[m];
                func_name[k] = '\0';
                
                
                if(strcmp(func_name, "if") != 0 && strcmp(func_name, "while") != 0 && 
                   strcmp(func_name, "for") != 0 && strcmp(func_name, "switch") != 0 && 
                   strcmp(func_name, "return") != 0 && strcmp(func_name, "sizeof") != 0) {
                    add_symbol(func_name, "fn()", 1, "");
                }
            }
        }

        
        if (text[i] == '#' && i+7 < len && my_strncmp(&text[i], "#define", 7) == 0) {
            i += 7; while(i < len && is_space_c(text[i])) i++;
            char mname[64]; int k = 0; while(i < len && is_alpha_num_uscore(text[i]) && k < 63) mname[k++] = text[i++];
            mname[k] = '\0';
            if (k > 0) add_symbol(mname, "macro", 3, "");
        }

        
        if (text[i] == '=') {
            int j = i - 1;
            while(j >= 0 && is_space_c(text[j])) j--;
            if (j >= 0 && is_alpha_num_uscore(text[j])) {
                int end = j;
                while(j >= 0 && is_alpha_num_uscore(text[j])) j--;
                int start = j + 1;
                char var_name[64]; int k = 0;
                for(int m = start; m <= end && k < 63; m++) var_name[k++] = text[m];
                var_name[k] = '\0';
                
                if (text[i-1] != '=' && text[i-1] != '>' && text[i-1] != '<' && text[i-1] != '!') {
                    add_symbol(var_name, "var", 2, "");
                }
            }
        }

        i++;
    }
}

void safe_insert_char(char *line, int *cx, char c) { int len = strlen(line); if (*cx > len) *cx = len; if (len >= MAX_LINE_LEN - 1) return; for (int i = len; i >= *cx; i--) { if (i + 1 < MAX_LINE_LEN) line[i + 1] = line[i]; } line[*cx] = c; (*cx)++; line[MAX_LINE_LEN - 1] = '\0'; }
void safe_delete_char(char *line, int *cx) { int len = strlen(line); if (*cx <= 0 || len == 0) return; for (int i = *cx; i <= len; i++) { line[i - 1] = line[i]; } (*cx)--; }
void safe_split_line(char *line, int cx, char *new_line) { int len = strlen(line); if (cx > len) cx = len; for (int i = cx; i <= len; i++) new_line[i - cx] = line[i]; line[cx] = '\0'; }
void print_spaces(int count) { for(int i = 0; i < count; i++) print_char(' '); }

typedef struct { char name[64]; int type; } TreeItem;
TreeItem tree[128]; int tree_cnt = 0; int tree_sel = 0; int tree_offset = 0;
void load_tree() { tree_cnt = 0; vfs_dirent_t ent; int idx = 0; if (strcmp(current_dir, "/") != 0) { strcpy(tree[tree_cnt].name, ".."); tree[tree_cnt].type = VFS_ATTR_DIR; tree_cnt++; } while (readdir(current_dir, idx++, &ent) == 1) { if (tree_cnt < 128) { strcpy(tree[tree_cnt].name, ent.name); tree[tree_cnt].type = ent.type; tree_cnt++; } } tree_sel = 0; tree_offset = 0; }

typedef struct { char **lines; uint8_t *allocs; int num_lines; int capacity; int cx, cy; int row_offset, col_offset; char filename[64]; int readonly; } Editor;
Editor eds[2];
Editor term_buf; char term_input[256]; int term_cx = 0;

void init_editor(Editor *ed) { ed->capacity = 100; ed->num_lines = 1; ed->lines = (char**)malloc(ed->capacity * sizeof(char*)); ed->allocs = (uint8_t*)malloc(ed->capacity * sizeof(uint8_t)); ed->lines[0] = (char*)malloc(MAX_LINE_LEN); ed->lines[0][0] = '\0'; ed->allocs[0] = 1; ed->cx = 0; ed->cy = 0; ed->row_offset = 0; ed->col_offset = 0; ed->readonly = 0; strcpy(ed->filename, "untitled.fc"); }
void clear_editor(Editor *ed) { for(int i = 0; i < ed->num_lines; i++) if(ed->allocs[i]) free(ed->lines[i]); ed->num_lines = 1; ed->lines[0] = (char*)malloc(MAX_LINE_LEN); ed->lines[0][0] = '\0'; ed->allocs[0] = 1; ed->cx = 0; ed->cy = 0; ed->row_offset = 0; ed->col_offset = 0; ed->readonly = 0; }
void add_line_to_editor(Editor *ed, char *text) { if (ed->num_lines >= ed->capacity) { ed->capacity *= 2; char **nlines = (char**)malloc(ed->capacity * sizeof(char*)); uint8_t *nallocs = (uint8_t*)malloc(ed->capacity * sizeof(uint8_t)); for(int i=0; i < ed->num_lines; i++) { nlines[i] = ed->lines[i]; nallocs[i] = ed->allocs[i]; } free(ed->lines); free(ed->allocs); ed->lines = nlines; ed->allocs = nallocs; } ed->lines[ed->num_lines] = (char*)malloc(MAX_LINE_LEN); strcpy(ed->lines[ed->num_lines], text); ed->allocs[ed->num_lines] = 1; ed->num_lines++; }
void int_to_padded_str(int val, char *buf) { char temp[16]; int i = 0; if (val == 0) temp[i++] = '0'; else while(val > 0) { temp[i++] = (val % 10) + '0'; val /= 10; } int padding = 4 - i; if (padding < 0) padding = 0; int pos = 0; while(padding--) buf[pos++] = ' '; while(i > 0) buf[pos++] = temp[--i]; buf[pos] = '\0'; }

void rescan_editor(Editor *ed) {
    if (!ed || ed->num_lines == 0) return;
    int total_size = 0; for(int i = 0; i < ed->num_lines; i++) total_size += strlen(ed->lines[i]) + 1;
    char *tmp_buf = malloc(total_size + 1); if (!tmp_buf) return;
    int pos = 0; for(int i = 0; i < ed->num_lines; i++) { int len = strlen(ed->lines[i]); my_strncpy(&tmp_buf[pos], ed->lines[i], len); pos += len; tmp_buf[pos++] = '\n'; } tmp_buf[pos] = '\0';
    init_base_symbols(); scan_buffer_for_symbols(tmp_buf, pos); free(tmp_buf);
}

void validate_active_panel() {
    if (active_panel == PANEL_TREE && !show_tree) active_panel = PANEL_ED1;
    if (active_panel == PANEL_TERM && !show_term) active_panel = PANEL_ED1;
    if (active_panel == PANEL_ED2 && split_mode == 0) active_panel = PANEL_ED1;
}

void update_layout() {
    int avail_w = term_cols;
    int avail_h = term_rows - 1; 
    int curr_x = 0; int curr_y = 0;

    if (show_tree) {
        v_tree.active = 1; v_tree.x = 0; v_tree.y = 0; v_tree.w = TREE_WIDTH; v_tree.h = avail_h;
        curr_x += TREE_WIDTH + 1; avail_w -= TREE_WIDTH + 1;
    } else v_tree.active = 0;

    if (show_term) {
        v_term.active = 1; v_term.h = avail_h / 3; if (v_term.h < 5) v_term.h = 5;
        v_term.w = avail_w; v_term.x = curr_x; v_term.y = avail_h - v_term.h;
        avail_h -= v_term.h;
    } else v_term.active = 0;

    v_ed1.active = 1;
    if (split_mode == 1) { 
        v_ed1.x = curr_x; v_ed1.y = curr_y; v_ed1.w = avail_w / 2; v_ed1.h = avail_h;
        v_ed2.active = 1; v_ed2.x = curr_x + v_ed1.w + 1; v_ed2.y = curr_y; v_ed2.w = avail_w - v_ed1.w - 1; v_ed2.h = avail_h;
    } else if (split_mode == 2) { 
        v_ed1.x = curr_x; v_ed1.y = curr_y; v_ed1.w = avail_w; v_ed1.h = avail_h / 2;
        v_ed2.active = 1; v_ed2.x = curr_x; v_ed2.y = curr_y + v_ed1.h; v_ed2.w = avail_w; v_ed2.h = avail_h - v_ed1.h;
    } else { 
        v_ed1.x = curr_x; v_ed1.y = curr_y; v_ed1.w = avail_w; v_ed1.h = avail_h; v_ed2.active = 0;
    }
    validate_active_panel();
}

int decode_inst(uint8_t *code, int offset, int max_len, uint32_t vaddr, char *asm_s, char *c_s, char *fc_s) {
    uint8_t b = code[offset];
    if (b == 0xB8 && max_len >= 5) { uint32_t imm = *(uint32_t*)&code[offset+1]; if (imm > 0x10000) { char h[16]; hex_32_str(imm, h); sprintf(asm_s, "mov eax, 0x%s", h); sprintf(c_s, "eax = 0x%s;", h); sprintf(fc_s, "eax = 0x%s;", h); } else { sprintf(asm_s, "mov eax, %d", imm); sprintf(c_s, "eax = %d;", imm); sprintf(fc_s, "eax = %d;", imm); } return 5; }
    if (b == 0xB9 && max_len >= 5) { uint32_t imm = *(uint32_t*)&code[offset+1]; sprintf(asm_s, "mov ecx, %d", imm); sprintf(c_s, "ecx = %d;", imm); sprintf(fc_s, "ecx = %d;", imm); return 5; }
    if (b == 0x8D && code[offset+1] == 0x85 && max_len >= 6) { int32_t disp = *(int32_t*)&code[offset+2]; sprintf(asm_s, "lea eax, [ebp%s%d]", disp<0?"":"+", disp); sprintf(c_s, "eax = &local_%d;", disp<0?-disp:disp); sprintf(fc_s, "eax = &l_%d;", disp<0?-disp:disp); return 6; }
    if (b == 0x05 && max_len >= 5) { uint32_t imm = *(uint32_t*)&code[offset+1]; sprintf(asm_s, "add eax, %d", imm); sprintf(c_s, "eax += %d;", imm); sprintf(fc_s, "eax += %d;", imm); return 5; }
    if (b == 0xE8 && max_len >= 5) { int32_t rel = *(int32_t*)&code[offset+1]; uint32_t target = vaddr + offset + 5 + rel; char h[16]; hex_32_str(target, h); sprintf(asm_s, "call 0x%s", h); strcpy(c_s, "func();"); strcpy(fc_s, "fn();"); return 5; }
    if (b == 0xE9 && max_len >= 5) { int32_t rel = *(int32_t*)&code[offset+1]; uint32_t target = vaddr + offset + 5 + rel; char h[16]; hex_32_str(target, h); sprintf(asm_s, "jmp 0x%s", h); strcpy(c_s, "goto;"); strcpy(fc_s, "jump;"); return 5; }
    if (max_len >= 2) { uint8_t b2 = code[offset+1]; if (b == 0x89 && b2 == 0xE5) { strcpy(asm_s, "mov ebp, esp"); strcpy(c_s, "ebp=esp;"); strcpy(fc_s, "ebp=esp;"); return 2; } if (b == 0xCD && b2 == 0x80) { strcpy(asm_s, "int 0x80"); strcpy(c_s, "syscall();"); strcpy(fc_s, "syscall();"); return 2; } }
    if (b == 0xC3) { strcpy(asm_s, "ret"); strcpy(c_s, "return;"); strcpy(fc_s, "return;"); return 1; } 
    char hex[4]; hex_8_str(b, hex); sprintf(asm_s, "db 0x%s", hex); strcpy(c_s, "???"); strcpy(fc_s, "???"); return 1;
}

void open_file(char* name) {
    char full[256];
    if (strcmp(current_dir, "/") == 0) sprintf(full, "/%s", name); else sprintf(full, "%s/%s", current_dir, name);
    int size = get_file_size(full); if (size < 0) size = 0; 
    uint8_t *buf = malloc(size + 1); if (size > 0) read_file(full, buf); buf[size] = '\0';
    clear_editor(&eds[0]); strcpy(eds[0].filename, name); eds[0].num_lines = 0;
    
    int is_elf = (size >= 16 && buf[0] == 0x7F && buf[1] == 'E' && buf[2] == 'L' && buf[3] == 'F');
    int is_flat_bin = (!is_elf && is_executable_ext(name));

    if (is_elf || is_flat_bin) {
        eds[0].readonly = 1;
        uint32_t code_offset = 0, code_size = size, vaddr = 0x40000000;
        add_line_to_editor(&eds[0], "ADDRESS    | ASM X86 INSTRUCTION      | C EQUIVALENT             | FC EQUIVALENT");
        add_line_to_editor(&eds[0], "-----------+--------------------------+--------------------------+--------------------------");
        int pc = 0;
        while (pc < code_size && pc < 10000) { 
            char asm_s[64] = {0}, c_s[64] = {0}, fc_s[64] = {0};
            int len = decode_inst(buf + code_offset, pc, code_size - pc, vaddr, asm_s, c_s, fc_s);
            char p_asm[30], p_c[30], p_fc[30], hex_addr[16];
            pad_string(p_asm, asm_s, 24); pad_string(p_c, c_s, 24); pad_string(p_fc, fc_s, 24); hex_32_str(vaddr + pc, hex_addr);
            char final_line[256]; sprintf(final_line, "0x%s | %s | %s | %s", hex_addr, p_asm, p_c, p_fc);
            add_line_to_editor(&eds[0], final_line); pc += len;
        }
    } else {
        int i = 0, j = 0; char temp[MAX_LINE_LEN];
        while(i < size) {
            if(buf[i] == '\n') { temp[j] = '\0'; add_line_to_editor(&eds[0], temp); j = 0; } 
            else if (buf[i] != '\r' && j < MAX_LINE_LEN - 1) { temp[j++] = buf[i]; } i++;
        }
        if(j > 0 || size == 0) { temp[j] = '\0'; add_line_to_editor(&eds[0], temp); }
        rescan_editor(&eds[0]);
    }
    active_panel = PANEL_ED1; free(buf);
}

void draw_editor(Editor *ed, int sx, int sy, int w, int h, int is_active) {
    if (w <= 0 || h <= 0) return;
    set_cursor(sx, sy);
    if (is_active) set_color(COLOR_WHITE, COLOR_BLUE); else set_color(COLOR_LIGHT_GRAY, COLOR_DARK_GRAY);
    char head[128]; sprintf(head, " %s ", ed->filename); printf("%s", head); 
    
    
    int h_fill = w - strlen(head);
    if (sx + w >= term_cols) h_fill--; 
    if (h_fill > 0) print_spaces(h_fill);

    for(int r = 0; r < h - 1; r++) {
        set_cursor(sx, sy + r + 1); int l_idx = ed->row_offset + r;
        if (l_idx < ed->num_lines) {
            set_color(COLOR_DARK_GRAY, COLOR_BLACK);
            char num_buf[8]; int_to_padded_str(l_idx + 1, num_buf); printf("%s|", num_buf);
            int t_len = strlen(ed->lines[l_idx]); int printed = 0; int in_string = 0, in_char = 0, in_comment = 0;
            for(int c = ed->col_offset; c < t_len && printed < w - 6; ) {
                char ch = ed->lines[l_idx][c];
                if (ch == '\t') { set_color(COLOR_WHITE, COLOR_BLACK); print_char(' '); printed++; while(printed % 4 != 0 && printed < w - 6) { print_char(' '); printed++; } c++; continue; }
                if (in_comment) { set_color(COLOR_DARK_GRAY, COLOR_BLACK); print_char(ch); c++; printed++; continue; }
                if (ch == '/' && ed->lines[l_idx][c+1] == '/') { in_comment = 1; set_color(COLOR_DARK_GRAY, COLOR_BLACK); print_char(ch); c++; printed++; continue; }
                if (ch == '"' && !in_char) { in_string = !in_string; set_color(COLOR_YELLOW, COLOR_BLACK); print_char(ch); c++; printed++; continue; }
                if (ch == '\'' && !in_string) { in_char = !in_char; set_color(COLOR_YELLOW, COLOR_BLACK); print_char(ch); c++; printed++; continue; }
                if (in_string || in_char) { set_color(COLOR_YELLOW, COLOR_BLACK); print_char(ch); c++; printed++; continue; }

                int is_bound = (c == 0) || !is_alpha_num_uscore(ed->lines[l_idx][c-1]);
                if (is_bound && is_alpha_num_uscore(ch)) {
                    char *ww = &ed->lines[l_idx][c]; int kw_len = 0;
                    #define CHECK_KW(kw, len) if (my_strncmp(ww, kw, len) == 0 && !is_alpha_num_uscore(ww[len])) kw_len = len
                    CHECK_KW("int", 3); else CHECK_KW("void", 4); else CHECK_KW("char", 4); else CHECK_KW("string", 6);
                    else CHECK_KW("return", 6); else CHECK_KW("if", 2); else CHECK_KW("else", 4);
                    else CHECK_KW("while", 5); else CHECK_KW("for", 3); else CHECK_KW("struct", 6);
                    else CHECK_KW("fn", 2); else CHECK_KW("let", 3); else CHECK_KW("mut", 3); else CHECK_KW("impl", 4); else CHECK_KW("pub", 3);
                    if (kw_len > 0) { set_color(COLOR_LIGHT_BLUE, COLOR_BLACK); for(int k=0; k<kw_len && printed < w - 6; k++) { print_char(ed->lines[l_idx][c++]); printed++; } continue; }
                }
                if (ch >= '0' && ch <= '9') set_color(COLOR_LIGHT_MAGENTA, COLOR_BLACK);
                else if (ch == '(' || ch == ')' || ch == '{' || ch == '}' || ch == '[' || ch == ']') set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
                else if (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '=' || ch == '<' || ch == '>' || ch == '&' || ch == '|' || ch == '!' || ch == ';' || ch == ',' || ch == '.' || ch == '#') set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
                else set_color(COLOR_WHITE, COLOR_BLACK);
                print_char(ch); c++; printed++;
            }
            int l_fill = w - 5 - printed;
            if (sx + w >= term_cols) l_fill--; 
            if (l_fill > 0) print_spaces(l_fill);
        } else { 
            set_color(COLOR_DARK_GRAY, COLOR_BLACK); printf("   ~|"); 
            int empty_fill = w - 5;
            if (sx + w >= term_cols) empty_fill--; 
            if (empty_fill > 0) print_spaces(empty_fill); 
        }
        if (sx + w < term_cols) { set_cursor(sx + w - 1, sy + r + 1); set_color(COLOR_DARK_GRAY, COLOR_BLACK); print_char('|'); }
    }
}

void draw_tree(int sx, int sy, int w, int h) {
    if (w <= 0 || h <= 0) return;
    set_cursor(sx, sy); if (active_panel == PANEL_TREE) set_color(COLOR_WHITE, COLOR_BLUE); else set_color(COLOR_LIGHT_GRAY, COLOR_DARK_GRAY);
    printf(" EXPLORER"); 
    int h_fill = w - 9; if (sx + w >= term_cols) h_fill--;
    if (h_fill > 0) print_spaces(h_fill);

    for(int i=0; i < h - 1; i++) {
        set_cursor(sx, sy + i + 1); int idx = tree_offset + i;
        if (idx < tree_cnt) {
            if (idx == tree_sel && active_panel == PANEL_TREE) set_color(COLOR_BLACK, COLOR_LIGHT_GRAY);
            else set_color(tree[idx].type == VFS_ATTR_DIR ? COLOR_LIGHT_BLUE : COLOR_WHITE, COLOR_BLACK);
            char name[64]; strcpy(name, tree[idx].name); if (strlen(name) > w - 3) name[w - 3] = '\0';
            if (tree[idx].type == VFS_ATTR_DIR) printf(" /%s", name); else printf("  %s", name); 
            int r_fill = w - strlen(name) - 2; if (sx + w >= term_cols) r_fill--;
            if (r_fill > 0) print_spaces(r_fill);
        } else { 
            set_color(COLOR_BLACK, COLOR_BLACK); 
            int empty_fill = w; if (sx + w >= term_cols) empty_fill--;
            if (empty_fill > 0) print_spaces(empty_fill); 
        }
        if (sx + w < term_cols) { set_cursor(sx + w - 1, sy + i + 1); set_color(COLOR_DARK_GRAY, COLOR_BLACK); print_char('|'); }
    }
}

void draw_terminal(int sx, int sy, int w, int h, int is_active) {
    if (w <= 0 || h <= 0) return;
    set_cursor(sx, sy);
    if (is_active) set_color(COLOR_WHITE, COLOR_BLUE); else set_color(COLOR_LIGHT_GRAY, COLOR_DARK_GRAY);
    printf(" TERMINAL"); 
    int h_fill = w - 9; if (sx + w >= term_cols) h_fill--;
    if (h_fill > 0) print_spaces(h_fill);

    int out_h = h - 2;
    for(int r=0; r<out_h; r++) {
        set_cursor(sx, sy + 1 + r);
        int l_idx = term_buf.row_offset + r;
        set_color(COLOR_LIGHT_GRAY, COLOR_BLACK);
        if (l_idx < term_buf.num_lines) {
            int len = strlen(term_buf.lines[l_idx]); int printed = len > w ? w : len;
            char tmp[256]; my_strncpy(tmp, term_buf.lines[l_idx], printed); tmp[printed]='\0';
            printf("%s", tmp); 
            int r_fill = w - printed; if (sx + w >= term_cols) r_fill--;
            if (r_fill > 0) print_spaces(r_fill);
        } else {
            int empty_fill = w; if (sx + w >= term_cols) empty_fill--;
            if (empty_fill > 0) print_spaces(empty_fill);
        }
    }
    set_cursor(sx, sy + h - 1); set_color(COLOR_GREEN, COLOR_BLACK); printf("> ");
    set_color(COLOR_WHITE, COLOR_BLACK);
    int printed = strlen(term_input); if (printed > w - 3) printed = w - 3;
    char tmp[256]; my_strncpy(tmp, term_input, printed); tmp[printed]='\0';
    printf("%s", tmp); 
    int in_fill = w - 2 - printed; if (sx + w >= term_cols) in_fill--;
    if (in_fill > 0) print_spaces(in_fill);
}


void free_parse_str(char **args) {
    if (args == NULL) return;

    int i = 0;
    
    while (args[i] != NULL) {
        free(args[i]);
        i++;
    }

    
    free(args);
}

void run_term_cmd() {
    if (term_input[0] == '\0') return;
    char echo_cmd[256]; sprintf(echo_cmd, "> %s", term_input);
    add_line_to_editor(&term_buf, echo_cmd);
    
    char **args = parse_str(term_input, ' ');
    if (args && args[0]) {
        char resolved[128];
        sprintf(resolved, "/path/%s.elf", args[0]);
        if (get_file_size(resolved) <= 0) sprintf(resolved, "/path/%s", args[0]);
        
        int pid = spawn(resolved, args, "/tmp/term.out");
        if (pid > 0) {
            waitpid(pid);
            int sz = get_file_size("/tmp/term.out");
            if (sz > 0) {
                uint8_t* out = malloc(sz+1); read_file("/tmp/term.out", out); out[sz] = 0;
                char* line = (char*)out;
                while(*line) {
                    char* end = line; while(*end && *end != '\n') end++;
                    if (*end) { *end = '\0'; end++; }
                    add_line_to_editor(&term_buf, line); line = end;
                }
                free(out); delete_file("/tmp/term.out");
            }
        } else add_line_to_editor(&term_buf, "Command not found.");
    }
    term_input[0] = '\0'; term_cx = 0;
    if (term_buf.num_lines > v_term.h - 2) term_buf.row_offset = term_buf.num_lines - (v_term.h - 2);
    free_parse_str(args);
}

void draw_autocomplete(int screen_x, int screen_y) {
    if(!ac.active || ac.count == 0) return;
    int box_width = 30; int box_height = ac.count > 6 ? 6 : ac.count;
    if (screen_x + box_width > term_cols) screen_x = term_cols - box_width;
    if (screen_y + box_height >= term_rows - 1) screen_y -= (box_height + 1); 
    for(int i=0; i<box_height; i++) {
        int idx = ac.offset + i;
        if (idx >= ac.count) break;
        set_cursor(screen_x, screen_y + i + 1);
        if (idx == ac.selected) set_color(COLOR_BLACK, COLOR_LIGHT_CYAN); else set_color(COLOR_LIGHT_GRAY, COLOR_DARK_GRAY);
        char line[64]; Symbol* s = ac.matches[idx];
        char type_char = s->type == 0 ? 'K' : (s->type == 1 ? 'F' : (s->type == 2 ? 'M' : '#'));
        sprintf(line, "[%c] %s", type_char, s->name);
        int len = strlen(line); printf("%s", line); print_spaces(box_width - len);
        if (s->type == 1 || s->type == 2) {
            int sig_len = strlen(s->signature); if (sig_len > 12) sig_len = 12; 
            set_cursor(screen_x + box_width - sig_len - 1, screen_y + i + 1);
            if (idx == ac.selected) set_color(COLOR_DARK_GRAY, COLOR_LIGHT_CYAN); else set_color(COLOR_LIGHT_BLUE, COLOR_DARK_GRAY);
            char short_sig[16]; my_strncpy(short_sig, s->signature, sig_len); short_sig[sig_len] = '\0'; printf("%s", short_sig);
        }
    }
}

void update_autocomplete(Editor *ed) {
    char* line = ed->lines[ed->cy]; int start = ed->cx - 1;
    while(start >= 0 && is_alpha_num_uscore(line[start])) start--;
    if (start >= 0 && line[start] == '.') {
        ac.is_dot_context = 1; int len = ed->cx - (start + 1);
        my_strncpy(ac.current_word, &line[start + 1], len); ac.current_word[len] = '\0';
        int obj_end = start - 1; int obj_start = obj_end;
        while(obj_start >= 0 && is_alpha_num_uscore(line[obj_start])) obj_start--; obj_start++;
        char obj_name[64]; my_strncpy(obj_name, &line[obj_start], obj_end - obj_start + 1); obj_name[obj_end - obj_start + 1] = '\0';
        char obj_type[64] = "";
        for(int i=0; i<local_cnt; i++) if (strcmp(locals[i].var_name, obj_name) == 0) { strcpy(obj_type, locals[i].type_name); break; }
        ac.count = 0;
        for(int i=0; i<symbol_cnt; i++) {
            if (global_symbols[i].type == 2 && strcmp(global_symbols[i].parent, obj_type) == 0) {
                if (my_strncmp(global_symbols[i].name, ac.current_word, len) == 0) { ac.matches[ac.count++] = &global_symbols[i]; if(ac.count >= MAX_SUGGESTIONS) break; }
            }
        }
        if(ac.count > 0) { ac.active = 1; ac.selected = 0; } else ac.active = 0; return;
    }
    start++; ac.is_dot_context = 0; int len = ed->cx - start; if (len <= 0) { ac.active = 0; return; }
    my_strncpy(ac.current_word, &line[start], len); ac.current_word[len] = '\0'; ac.count = 0;
    for(int i=0; i<symbol_cnt; i++) {
        if(global_symbols[i].type != 2 && my_strncmp(global_symbols[i].name, ac.current_word, len) == 0) {
            ac.matches[ac.count++] = &global_symbols[i]; if(ac.count >= MAX_SUGGESTIONS) break;
        }
    }
    if(ac.count > 0) { ac.active = 1; ac.selected = 0; ac.offset = 0; } else ac.active = 0;
}

void insert_autocomplete(Editor *ed) {
    if(!ac.active || ac.count == 0) return;
    Symbol* s = ac.matches[ac.selected]; int len = strlen(ac.current_word);
    for(int i=0; i<len; i++) safe_delete_char(ed->lines[ed->cy], &ed->cx);
    for(int i=0; s->name[i] != '\0'; i++) safe_insert_char(ed->lines[ed->cy], &ed->cx, s->name[i]);
    if (s->type == 1 || (s->type == 2 && strcmp(s->signature, "field") != 0)) { safe_insert_char(ed->lines[ed->cy], &ed->cx, '('); safe_insert_char(ed->lines[ed->cy], &ed->cx, ')'); ed->cx--; }
    ac.active = 0; rescan_editor(ed);
}

void draw_screen() {
    update_layout();
    if (v_tree.active) draw_tree(v_tree.x, v_tree.y, v_tree.w, v_tree.h);
    if (v_ed1.active) draw_editor(&eds[0], v_ed1.x, v_ed1.y, v_ed1.w, v_ed1.h, active_panel == PANEL_ED1);
    if (v_ed2.active) draw_editor(&eds[1], v_ed2.x, v_ed2.y, v_ed2.w, v_ed2.h, active_panel == PANEL_ED2);
    if (v_term.active) draw_terminal(v_term.x, v_term.y, v_term.w, v_term.h, active_panel == PANEL_TERM);

    set_cursor(0, term_rows - 1); set_color(COLOR_BLACK, COLOR_LIGHT_GRAY);
    char footer[128]; sprintf(footer, " Ctrl+Q:Quit | Ctrl+E:Tree | Ctrl+S:Split | Ctrl+T:Term | Ctrl+W:Pane | F2:Save ");
    printf("%s", footer); 
    
    
    int f_fill = term_cols - strlen(footer) - 1; 
    if (f_fill > 0) print_spaces(f_fill);
}

int main(int argc, char** argv) {
    get_term_size(&term_cols, &term_rows); if(term_cols == 0) { term_cols = 80; term_rows = 25; }
    init_editor(&eds[0]); init_editor(&eds[1]); 
    init_editor(&term_buf); term_buf.readonly = 1; strcpy(term_buf.filename, "Terminal"); term_input[0] = '\0';
    init_base_symbols(); load_tree();
    if(argc > 1) open_file(argv[1]);
    clear_screen();

    while(1) {
        draw_screen();
        int screen_x = 0, screen_y = 0;
        if (active_panel == PANEL_ED1 || active_panel == PANEL_ED2) {
            Editor *ed = &eds[active_panel - 1]; ViewRect *vr = (active_panel == PANEL_ED1) ? &v_ed1 : &v_ed2;
            int visual_cx = 0;
            for(int i = ed->col_offset; i < ed->cx && ed->lines[ed->cy][i] != '\0'; i++) {
                if (ed->lines[ed->cy][i] == '\t') { visual_cx++; while(visual_cx % 4 != 0) visual_cx++; } else visual_cx++;
            }
            screen_x = vr->x + visual_cx + 5; screen_y = vr->y + ed->cy - ed->row_offset + 1;
            draw_autocomplete(screen_x, screen_y); 
            set_cursor(screen_x, screen_y); set_color(COLOR_BLACK, COLOR_WHITE);
            char c = ed->lines[ed->cy][ed->cx]; print_char((c == '\0' || c == '\n') ? ' ' : c);
        } else if (active_panel == PANEL_TERM) {
            screen_x = v_term.x + 2 + term_cx; screen_y = v_term.y + v_term.h - 1;
            set_cursor(screen_x, screen_y); set_color(COLOR_BLACK, COLOR_WHITE);
            char c = term_input[term_cx]; print_char((c == '\0') ? ' ' : c);
        }

        unsigned int c = getc();
        uint8_t mods = get_key_modifiers();
        int is_ctrl = mods & (1 << 1);

        if (is_ctrl) {
            if (c == 'e' || c == 'E') show_tree = !show_tree;
            else if (c == 't' || c == 'T') show_term = !show_term;
            else if (c == 's' || c == 'S') split_mode = (split_mode + 1) % 3;
            else if (c == 'q' || c == 'Q') break;
            else if (c == 'w' || c == 'W') {
                do { active_panel = (active_panel + 1) % 4; } while (
                    (active_panel == PANEL_TREE && !show_tree) ||
                    (active_panel == PANEL_ED2 && split_mode == 0) ||
                    (active_panel == PANEL_TERM && !show_term)
                );
            }
            ac.active = 0; update_layout(); continue;
        }

        if (ac.active && (active_panel == PANEL_ED1 || active_panel == PANEL_ED2)) {
            int max_vis = 6;
            if (c == KEY_DOWN) { 
                ac.selected = (ac.selected + 1) % ac.count; 
                if (ac.selected == 0) ac.offset = 0; 
                else if (ac.selected >= ac.offset + max_vis) ac.offset = ac.selected - max_vis + 1;
                continue; 
            }
            else if (c == KEY_UP) { 
                ac.selected = (ac.selected - 1 < 0) ? ac.count - 1 : ac.selected - 1; 
                if (ac.selected == ac.count - 1) ac.offset = ac.count > max_vis ? ac.count - max_vis : 0;
                else if (ac.selected < ac.offset) ac.offset = ac.selected;
                continue; 
            }
            else if (c == KEY_TAB || c == KEY_ENTER) { insert_autocomplete(&eds[active_panel-1]); continue; }
            else if (c == KEY_ESC) { ac.active = 0; continue; }
        }

        if (c == KEY_ESC) break;
        else if (active_panel == PANEL_TREE) { 
            if (c == KEY_UP && tree_sel > 0) tree_sel--; else if (c == KEY_DOWN && tree_sel < tree_cnt - 1) tree_sel++;
            else if (c == KEY_ENTER) {
                if (tree[tree_sel].type == VFS_ATTR_DIR) {
                    if (strcmp(tree[tree_sel].name, "..") == 0) { for(int i = strlen(current_dir)-1; i >= 0; i--) { if (current_dir[i] == '/') { current_dir[i] = '\0'; break; } } if (strlen(current_dir) == 0) strcpy(current_dir, "/"); } 
                    else { if (strcmp(current_dir, "/") != 0) strcat(current_dir, "/"); strcat(current_dir, tree[tree_sel].name); } load_tree();
                } else open_file(tree[tree_sel].name);
            }
            if (tree_sel < tree_offset) tree_offset = tree_sel; if (tree_sel >= tree_offset + v_tree.h - 2) tree_offset = tree_sel - (v_tree.h - 3);
        }
        else if (active_panel == PANEL_TERM) {
            int len = strlen(term_input);
            if (c == KEY_LEFT && term_cx > 0) term_cx--;
            else if (c == KEY_RIGHT && term_cx < len) term_cx++;
            else if (c == '\b' && term_cx > 0) { for(int i=term_cx; i<=len; i++) term_input[i-1] = term_input[i]; term_cx--; }
            else if (c == '\n') run_term_cmd();
            else if (c >= 32 && c <= 126 && len < 254) { for(int i=len; i>=term_cx; i--) term_input[i+1] = term_input[i]; term_input[term_cx++] = c; }
        }
        else { 
            Editor *ed = &eds[active_panel - 1]; ViewRect *vr = (active_panel == PANEL_ED1) ? &v_ed1 : &v_ed2;
            int len = strlen(ed->lines[ed->cy]); if (ed->cx > len) ed->cx = len;
            if (c == KEY_UP && ed->cy > 0) { ed->cy--; ac.active = 0; } else if (c == KEY_DOWN && ed->cy < ed->num_lines - 1) { ed->cy++; ac.active = 0; }
            else if (c == KEY_LEFT && ed->cx > 0) { ed->cx--; update_autocomplete(ed); } else if (c == KEY_RIGHT && ed->cx < strlen(ed->lines[ed->cy])) { ed->cx++; update_autocomplete(ed); }
            else if (c == '\b' && !ed->readonly) { 
                if (ed->cx > 0) safe_delete_char(ed->lines[ed->cy], &ed->cx);
                else if (ed->cy > 0) { 
                    int prev_len = strlen(ed->lines[ed->cy - 1]); int cur_len = strlen(ed->lines[ed->cy]);
                    if (prev_len + cur_len < MAX_LINE_LEN - 1) { strcat(ed->lines[ed->cy - 1], ed->lines[ed->cy]); free(ed->lines[ed->cy]); for (int i = ed->cy; i < ed->num_lines - 1; i++) { ed->lines[i] = ed->lines[i + 1]; ed->allocs[i] = ed->allocs[i + 1]; } ed->num_lines--; ed->cy--; ed->cx = prev_len; }
                } update_autocomplete(ed);
            }
            else if (c == '\n' && !ed->readonly) { 
                int indent = 0; for(int i=0; i<ed->cx; i++) { if(ed->lines[ed->cy][i] == ' ') indent++; else if(ed->lines[ed->cy][i] == '\t') indent += 4; else break; }
                int prev_brace = 0; for(int i=ed->cx-1; i>=0; i--) { if(!is_space_c(ed->lines[ed->cy][i])) { if(ed->lines[ed->cy][i] == '{') prev_brace = 1; break; } }
                if (prev_brace) indent += 4;

                char *new_line = malloc(MAX_LINE_LEN); safe_split_line(ed->lines[ed->cy], ed->cx, new_line); ed->lines[ed->cy][ed->cx] = '\0';
                char padded_line[MAX_LINE_LEN]; int p = 0; for(; p<indent && p<MAX_LINE_LEN-1; p++) padded_line[p] = ' '; strcpy(padded_line + p, new_line); free(new_line);
                new_line = malloc(MAX_LINE_LEN); strcpy(new_line, padded_line);

                if (ed->num_lines >= ed->capacity) { ed->capacity *= 2; char **nlines = malloc(ed->capacity * sizeof(char*)); uint8_t *nallocs = malloc(ed->capacity * sizeof(uint8_t)); for(int i=0; i < ed->num_lines; i++) { nlines[i] = ed->lines[i]; nallocs[i] = ed->allocs[i]; } free(ed->lines); free(ed->allocs); ed->lines = nlines; ed->allocs = nallocs; }
                for (int i = ed->num_lines; i > ed->cy + 1; i--) { ed->lines[i] = ed->lines[i - 1]; ed->allocs[i] = ed->allocs[i - 1]; }
                ed->lines[ed->cy + 1] = new_line; ed->allocs[ed->cy + 1] = 1; ed->num_lines++; ed->cy++; ed->cx = indent; ac.active = 0; rescan_editor(ed);
            }
            else if (c == KEY_F2 && !ed->readonly) { 
                int total_size = 0; for(int i = 0; i < ed->num_lines; i++) total_size += strlen(ed->lines[i]) + 1;
                uint8_t *save_buf = malloc(total_size + 1); int pos = 0;
                for(int i = 0; i < ed->num_lines; i++) { strcpy((char*)&save_buf[pos], ed->lines[i]); pos += strlen(ed->lines[i]); if (i < ed->num_lines - 1) save_buf[pos++] = '\n'; } save_buf[pos] = '\0';
                char full[256]; if (strcmp(current_dir, "/") == 0) sprintf(full, "/%s", ed->filename); else sprintf(full, "%s/%s", current_dir, ed->filename);
                write_file(full, save_buf, pos); free(save_buf); rescan_editor(ed);
            }
            else if (c == KEY_TAB && !ed->readonly){ for (int i = 0; i < 4; i++) safe_insert_char(ed->lines[ed->cy], &ed->cx, ' '); ac.active = 0; }
            else if (c >= 32 && c <= 126 && !ed->readonly) { 
                safe_insert_char(ed->lines[ed->cy], &ed->cx, c);
                if (c == '"' || c == '>') { if (my_strncmp(ed->lines[ed->cy], "#include", 8) == 0) { rescan_editor(ed); } }
                if(is_alpha_num_uscore(c) || c == '.' || c == '#') update_autocomplete(ed); else ac.active = 0; 
            }
            
            if(ed->cy < ed->row_offset) ed->row_offset = ed->cy; if(ed->cy >= ed->row_offset + vr->h - 2) ed->row_offset = ed->cy - (vr->h - 3);
            if(ed->cx < ed->col_offset) ed->col_offset = ed->cx;
            if(ed->cx >= ed->col_offset + vr->w - 6) ed->col_offset = ed->cx - (vr->w - 7);
        }
    }

    for(int i = 0; i < eds[0].num_lines; i++) if(eds[0].allocs[i]) free(eds[0].lines[i]); free(eds[0].lines); free(eds[0].allocs);
    for(int i = 0; i < eds[1].num_lines; i++) if(eds[1].allocs[i]) free(eds[1].lines[i]); free(eds[1].lines); free(eds[1].allocs);
    for(int i = 0; i < term_buf.num_lines; i++) if(term_buf.allocs[i]) free(term_buf.lines[i]); free(term_buf.lines); free(term_buf.allocs);
    clear_screen(); set_color(COLOR_WHITE, COLOR_BLACK); return 0;
}