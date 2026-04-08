#include <oslib.h>

#define MAX_LINE_LEN 256
#define TREE_WIDTH 20

#define PANEL_TREE 0
#define PANEL_ED1  1
#define PANEL_ED2  2

int active_panel = PANEL_TREE;
int is_split = 0; 
int term_cols = 80;
int term_rows = 25;

char current_dir[128] = "/";

#define MAX_LINE_LEN 256

void safe_insert_char(char *line, int *cx, char c) {
    int len = strlen(line);

    if (*cx > len) *cx = len;
    if (len >= MAX_LINE_LEN - 1) return;

    for (int i = len; i >= *cx; i--) {
        if (i + 1 < MAX_LINE_LEN)
            line[i + 1] = line[i];
    }

    line[*cx] = c;
    (*cx)++;

    line[MAX_LINE_LEN - 1] = '\0';
}

void safe_delete_char(char *line, int *cx) {
    int len = strlen(line);

    if (*cx <= 0 || len == 0) return;

    for (int i = *cx; i <= len; i++) {
        line[i - 1] = line[i];
    }

    (*cx)--;
}

void safe_split_line(char *line, int cx, char *new_line) {
    int len = strlen(line);

    if (cx > len) cx = len;

    
    for (int i = cx; i <= len; i++) {
        new_line[i - cx] = line[i];
    }

    
    line[cx] = '\0';
}

int my_strncmp(const char *s1, const char *s2, int n) {
    while (n > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return (*(unsigned char *)s1 - *(unsigned char *)s2);
}

typedef struct {
    char name[64];
    int type;
} TreeItem;

TreeItem tree[128];
int tree_cnt = 0;
int tree_sel = 0;
int tree_offset = 0;

void load_tree() {
    tree_cnt = 0;
    vfs_dirent_t ent;
    int idx = 0;
    
    if (strcmp(current_dir, "/") != 0) {
        strcpy(tree[tree_cnt].name, "..");
        tree[tree_cnt].type = VFS_ATTR_DIR;
        tree_cnt++;
    }

    while (readdir(current_dir, idx++, &ent) == 1) {
        if (tree_cnt < 128) {
            strcpy(tree[tree_cnt].name, ent.name);
            tree[tree_cnt].type = ent.type;
            tree_cnt++;
        }
    }
    tree_sel = 0;
    tree_offset = 0;
}

typedef struct {
    char **lines;
    uint8_t *allocs;
    int num_lines;
    int capacity;
    int cx, cy;
    int row_offset, col_offset;
    char filename[64];
    int readonly;
} Editor;

Editor eds[2]; 

void init_editor(Editor *ed) {
    ed->capacity = 100;
    ed->num_lines = 1;
    ed->lines = (char**)malloc(ed->capacity * sizeof(char*));
    ed->allocs = (uint8_t*)malloc(ed->capacity * sizeof(uint8_t));
    ed->lines[0] = (char*)malloc(MAX_LINE_LEN);
    ed->lines[0][0] = '\0';
    ed->allocs[0] = 1;
    ed->cx = 0; ed->cy = 0;
    ed->row_offset = 0; ed->col_offset = 0;
    ed->readonly = 0;
    strcpy(ed->filename, "untitled.c");
}

void clear_editor(Editor *ed) {
    for(int i = 0; i < ed->num_lines; i++) {
        if(ed->allocs[i]) free(ed->lines[i]);
    }
    ed->num_lines = 1;
    ed->lines[0] = (char*)malloc(MAX_LINE_LEN);
    ed->lines[0][0] = '\0';
    ed->allocs[0] = 1;
    ed->cx = 0; ed->cy = 0;
    ed->row_offset = 0; ed->col_offset = 0;
    ed->readonly = 0;
}

void add_line_to_editor(Editor *ed, char *text) {
    if (ed->num_lines >= ed->capacity) {
        ed->capacity *= 2;
        char **nlines = (char**)malloc(ed->capacity * sizeof(char*));
        uint8_t *nallocs = (uint8_t*)malloc(ed->capacity * sizeof(uint8_t));
        for(int i=0; i < ed->num_lines; i++) { nlines[i] = ed->lines[i]; nallocs[i] = ed->allocs[i]; }
        free(ed->lines); free(ed->allocs);
        ed->lines = nlines; ed->allocs = nallocs;
    }
    ed->lines[ed->num_lines] = (char*)malloc(strlen(text) + 1);
    strcpy(ed->lines[ed->num_lines], text);
    ed->allocs[ed->num_lines] = 1;
    ed->num_lines++;
}

void to_hex8(uint8_t val, char *out) {
    const char *hex = "0123456789ABCDEF";
    out[0] = hex[(val >> 4) & 0xF];
    out[1] = hex[val & 0xF];
    out[2] = '\0';
}

void to_hex32(uint32_t val, char *out) {
    const char *hex = "0123456789ABCDEF";
    for(int i=0; i<8; i++) {
        out[7-i] = hex[val & 0xF];
        val >>= 4;
    }
    out[8] = '\0';
}

void int_to_padded_str(int val, char *buf) {
    char temp[16]; int i = 0;
    if (val == 0) temp[i++] = '0';
    else while(val > 0) { temp[i++] = (val % 10) + '0'; val /= 10; }
    
    int padding = 4 - i;
    if (padding < 0) padding = 0;
    int pos = 0;
    while(padding--) buf[pos++] = ' ';
    while(i > 0) buf[pos++] = temp[--i];
    buf[pos] = '\0';
}

void load_hex_view(Editor *ed, uint8_t *data, int size) {
    clear_editor(ed);
    ed->num_lines = 0; 
    ed->readonly = 1;
    
    for (int i = 0; i < size; i += 16) {
        char line[128] = "";
        char hex[64] = "";
        char ascii[32] = "";
        
        char h32[9]; to_hex32(i, h32);
        strcpy(line, h32); strcat(line, ": ");

        for(int j=0; j<16; j++) {
            if (i + j < size) {
                char b[3]; to_hex8(data[i+j], b);
                strcat(hex, b); strcat(hex, " ");
                char c = data[i+j];
                if (c >= 32 && c <= 126) { char s[2] = {c, 0}; strcat(ascii, s); }
                else strcat(ascii, ".");
            } else {
                strcat(hex, "   ");
            }
        }
        strcat(line, hex); strcat(line, "| "); strcat(line, ascii);
        add_line_to_editor(ed, line);
    }
    if(ed->num_lines == 0) add_line_to_editor(ed, "EMPTY FILE");
}

void append_asm(char *line, uint32_t addr, const char *inst, uint32_t imm, int has_imm) {
    char h32[9]; to_hex32(addr, h32);
    strcpy(line, h32);
    strcat(line, ": "); strcat(line, inst);
    if (has_imm) {
        char himm[9]; to_hex32(imm, himm);
        char *p = himm; while(*p == '0' && *(p+1) != '\0') p++; 
        strcat(line, " 0x"); strcat(line, p);
    }
}

void load_asm_view(Editor *ed, uint8_t *data, int size) {
    clear_editor(ed);
    ed->num_lines = 0;
    ed->readonly = 1;
    
    int start_offset = 0;
    uint32_t base_addr = 0;
    int parse_size = size;

    
    if (size >= 52 && data[0] == 0x7F && data[1] == 'E' && data[2] == 'L' && data[3] == 'F') {
        uint32_t shoff = *(uint32_t*)(data + 32); 
        uint16_t shentsize = *(uint16_t*)(data + 46); 
        
        
        uint32_t text_shdr_offset = shoff + shentsize;
        
        if (text_shdr_offset + shentsize <= (uint32_t)size) {
            base_addr = *(uint32_t*)(data + text_shdr_offset + 12); 
            start_offset = *(uint32_t*)(data + text_shdr_offset + 16); 
            parse_size = *(uint32_t*)(data + text_shdr_offset + 20); 
        }
    }

    int pc = start_offset;
    int end_pc = start_offset + parse_size;
    if (end_pc > size) end_pc = size; 

    while (pc < end_pc) {
        uint8_t op = data[pc++];
        char line[128];
        uint32_t mem_addr = base_addr + (pc - 1 - start_offset);
        
        
        if (op == 0xB8 && pc + 3 < end_pc) { uint32_t imm = *(uint32_t*)&data[pc]; pc+=4; append_asm(line, mem_addr, "mov eax,", imm, 1); }
        else if (op == 0xA1 && pc + 3 < end_pc) { uint32_t imm = *(uint32_t*)&data[pc]; pc+=4; append_asm(line, mem_addr, "mov eax, [0x", imm, 1); strcat(line, "]"); }
        else if (op == 0xA3 && pc + 3 < end_pc) { uint32_t imm = *(uint32_t*)&data[pc]; pc+=4; append_asm(line, mem_addr, "mov [0x", imm, 1); strcat(line, "], eax"); }
        else if (op == 0xE8 && pc + 3 < end_pc) { int32_t rel = *(int32_t*)&data[pc]; pc+=4; append_asm(line, mem_addr, "call", mem_addr + 5 + rel, 1); }
        else if (op == 0xE9 && pc + 3 < end_pc) { int32_t rel = *(int32_t*)&data[pc]; pc+=4; append_asm(line, mem_addr, "jmp", mem_addr + 5 + rel, 1); }
        else if (op == 0x0F && pc < end_pc && data[pc] == 0x84 && pc + 4 < end_pc) { int32_t rel = *(int32_t*)&data[pc+1]; pc+=5; append_asm(line, mem_addr, "jz", mem_addr + 6 + rel, 1); }
        else if (op == 0x50) { append_asm(line, mem_addr, "push eax", 0, 0); }
        else if (op == 0x58) { append_asm(line, mem_addr, "pop eax", 0, 0); }
        else if (op == 0x59) { append_asm(line, mem_addr, "pop ecx", 0, 0); }
        else if (op == 0x5B) { append_asm(line, mem_addr, "pop ebx", 0, 0); }
        else if (op == 0x55) { append_asm(line, mem_addr, "push ebp", 0, 0); }
        else if (op == 0xC9) { append_asm(line, mem_addr, "leave", 0, 0); }
        else if (op == 0xC3) { append_asm(line, mem_addr, "ret", 0, 0); }
        else if (op == 0xCD && pc < end_pc && data[pc] == 0x80) { append_asm(line, mem_addr, "int 0x80", 0, 0); pc++; }
        else if (op == 0x89 && pc < end_pc && data[pc] == 0xE5) { append_asm(line, mem_addr, "mov ebp, esp", 0, 0); pc++; }
        else if (op == 0x89 && pc + 1 < end_pc && data[pc] == 0x45) { append_asm(line, mem_addr, "mov [ebp+off], eax", 0, 0); pc+=2; }
        else if (op == 0x8B && pc + 1 < end_pc && data[pc] == 0x45) { append_asm(line, mem_addr, "mov eax, [ebp+off]", 0, 0); pc+=2; }
        else if (op == 0x89 && pc < end_pc && data[pc] == 0xC8) { append_asm(line, mem_addr, "mov eax, ecx", 0, 0); pc++; }
        else if (op == 0x89 && pc < end_pc && data[pc] == 0xC2) { append_asm(line, mem_addr, "mov edx, eax", 0, 0); pc++; }
        else if (op == 0x01 && pc < end_pc && data[pc] == 0xC8) { append_asm(line, mem_addr, "add eax, ecx", 0, 0); pc++; }
        else if (op == 0x29 && pc < end_pc && data[pc] == 0xC1) { append_asm(line, mem_addr, "sub ecx, eax", 0, 0); pc++; }
        else if (op == 0x39 && pc < end_pc && data[pc] == 0xC1) { append_asm(line, mem_addr, "cmp ecx, eax", 0, 0); pc++; }
        else {
            char b[3]; to_hex8(op, b); 
            char h32[9]; to_hex32(mem_addr, h32);
            strcpy(line, h32); strcat(line, ": db 0x"); strcat(line, b);
        }
        add_line_to_editor(ed, line);
    }
    
    if(ed->num_lines == 0) add_line_to_editor(ed, "EMPTY BIN");
}

void open_file(char* name) {
    char full[256];
    if (strcmp(current_dir, "/") == 0) sprintf(full, "/%s", name);
    else sprintf(full, "%s/%s", current_dir, name);

    int size = get_file_size(full);
    if (size < 0) size = 0; 

    int len = strlen(name);
    int is_bin = 0;
    
    
    if (len >= 4) {
        char *ext = name + len - 4;
        if ((ext[0] == '.' && (ext[1] == 'e' || ext[1] == 'E') && (ext[2] == 'l' || ext[2] == 'L') && (ext[3] == 'f' || ext[3] == 'F')) ||
            (ext[0] == '.' && (ext[1] == 'b' || ext[1] == 'B') && (ext[2] == 'i' || ext[2] == 'I') && (ext[3] == 'n' || ext[3] == 'N'))) {
            is_bin = 1;
        }
    }

    uint8_t *buf = malloc(size + 1);
    if (size > 0) read_file(full, buf);
    buf[size] = '\0';

    if (is_bin && size > 0) {
        strcpy(eds[0].filename, name); strcat(eds[0].filename, " (ASM)");
        load_asm_view(&eds[0], buf, size);
        is_split = 0;
        active_panel = PANEL_ED1;
    } else {
        clear_editor(&eds[0]);
        strcpy(eds[0].filename, name);
        
        eds[0].num_lines = 0;
        int i = 0, j = 0; char temp[MAX_LINE_LEN];
        while(i < size) {
            if(buf[i] == '\n') {
                temp[j] = '\0';
                add_line_to_editor(&eds[0], temp);
                j = 0;
            } else if (buf[i] != '\r' && j < MAX_LINE_LEN - 1) {
                temp[j++] = buf[i];
            }
            i++;
        }
        if(j > 0 || size == 0) { temp[j] = '\0'; add_line_to_editor(&eds[0], temp); }
        
        is_split = 0;
        active_panel = PANEL_ED1;
    }
    free(buf);
}

void print_spaces(int count) { for(int i = 0; i < count; i++) print_char(' '); }

void draw_editor(Editor *ed, int start_x, int start_y, int width, int height, int is_active) {
    set_cursor(start_x, start_y);
    if (is_active) set_color(COLOR_WHITE, COLOR_BLUE);
    else set_color(COLOR_LIGHT_GRAY, COLOR_DARK_GRAY);
    
    char head[128]; 
    sprintf(head, " %s ", ed->filename); 
    printf("%s", head); 
    print_spaces(width - strlen(head));
    
    for(int r = 0; r < height - 1; r++) {
        set_cursor(start_x, start_y + r + 1);
        int l_idx = ed->row_offset + r;
        
        if (l_idx < ed->num_lines) {
            set_color(COLOR_DARK_GRAY, COLOR_BLACK);
            
            char num_buf[8];
            int_to_padded_str(l_idx + 1, num_buf);
            printf("%s|", num_buf);
            
            int t_len = strlen(ed->lines[l_idx]);
            int printed = 0;
            int in_string = 0, in_char = 0, in_comment = 0;

            for(int c = ed->col_offset; c < t_len && printed < width - 6; ) {
                char ch = ed->lines[l_idx][c];

                if (ch == '\t') {
                    set_color(COLOR_WHITE, COLOR_BLACK);
                    print_char(' '); printed++;
                    while(printed % 4 != 0 && printed < width - 6) { print_char(' '); printed++; }
                    c++; continue;
                }

                if (in_comment) {
                    set_color(COLOR_DARK_GRAY, COLOR_BLACK);
                    print_char(ch); c++; printed++; continue;
                }

                if (ch == '/' && ed->lines[l_idx][c+1] == '/') {
                    in_comment = 1;
                    set_color(COLOR_DARK_GRAY, COLOR_BLACK);
                    print_char(ch); c++; printed++; continue;
                }

                if (ch == '"' && !in_char) {
                    in_string = !in_string;
                    set_color(COLOR_YELLOW, COLOR_BLACK);
                    print_char(ch); c++; printed++; continue;
                }
                if (ch == '\'' && !in_string) {
                    in_char = !in_char;
                    set_color(COLOR_YELLOW, COLOR_BLACK);
                    print_char(ch); c++; printed++; continue;
                }
                if (in_string || in_char) {
                    set_color(COLOR_YELLOW, COLOR_BLACK);
                    print_char(ch); c++; printed++; continue;
                }

                
                int is_bound = (c == 0) || !( (ed->lines[l_idx][c-1] >= 'a' && ed->lines[l_idx][c-1] <= 'z') || 
                                              (ed->lines[l_idx][c-1] >= 'A' && ed->lines[l_idx][c-1] <= 'Z') || 
                                              (ed->lines[l_idx][c-1] >= '0' && ed->lines[l_idx][c-1] <= '9') || 
                                              ed->lines[l_idx][c-1] == '_' );
                
                if (is_bound && ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_')) {
                    char *w = &ed->lines[l_idx][c];
                    int kw_len = 0;
                    
                    #define CHECK_KW(kw, len) if (my_strncmp(w, kw, len) == 0 && !((w[len] >= 'a' && w[len] <= 'z') || (w[len] >= 'A' && w[len] <= 'Z') || (w[len] >= '0' && w[len] <= '9') || w[len] == '_')) kw_len = len
                    
                    CHECK_KW("int", 3); else CHECK_KW("void", 4); else CHECK_KW("char", 4);
                    else CHECK_KW("return", 6); else CHECK_KW("if", 2); else CHECK_KW("else", 4);
                    else CHECK_KW("while", 5); else CHECK_KW("for", 3); else CHECK_KW("struct", 6);
                    else CHECK_KW("typedef", 7); else CHECK_KW("sizeof", 6); else CHECK_KW("enum", 4);

                    if (kw_len > 0) {
                        set_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
                        for(int k=0; k<kw_len && printed < width - 6; k++) {
                            print_char(ed->lines[l_idx][c++]); printed++;
                        }
                        continue;
                    }
                }

                if (ch >= '0' && ch <= '9') set_color(COLOR_LIGHT_MAGENTA, COLOR_BLACK);
                else if (ch == '(' || ch == ')' || ch == '{' || ch == '}' || ch == '[' || ch == ']') set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
                else if (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '=' || ch == '<' || ch == '>' || ch == '&' || ch == '|' || ch == '!' || ch == ';' || ch == ',') set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
                else set_color(COLOR_WHITE, COLOR_BLACK);

                print_char(ch);
                c++; printed++;
            }
            print_spaces(width - 5 - printed);
        } else {
            set_color(COLOR_DARK_GRAY, COLOR_BLACK);
            printf("   ~|"); print_spaces(width - 5);
        }
        
        if (start_x + width < term_cols) {
            set_cursor(start_x + width, start_y + r + 1);
            set_color(COLOR_DARK_GRAY, COLOR_BLACK); 
            print_char('|');
        }
    }
}

void draw_tree(int height) {
    set_cursor(0, 0);
    if (active_panel == PANEL_TREE) set_color(COLOR_WHITE, COLOR_BLUE);
    else set_color(COLOR_LIGHT_GRAY, COLOR_DARK_GRAY);
    printf(" EXPLORER"); print_spaces(TREE_WIDTH - 9);
    
    for(int i=0; i < height - 1; i++) {
        set_cursor(0, i + 1);
        int idx = tree_offset + i;
        if (idx < tree_cnt) {
            if (idx == tree_sel && active_panel == PANEL_TREE) set_color(COLOR_BLACK, COLOR_LIGHT_GRAY);
            else set_color(tree[idx].type == VFS_ATTR_DIR ? COLOR_LIGHT_BLUE : COLOR_WHITE, COLOR_BLACK);
            
            char name[64]; strcpy(name, tree[idx].name);
            if (strlen(name) > TREE_WIDTH - 3) name[TREE_WIDTH - 3] = '\0';
            
            if (tree[idx].type == VFS_ATTR_DIR) printf(" /%s", name);
            else printf("  %s", name);
            
            print_spaces(TREE_WIDTH - strlen(name) - (tree[idx].type == VFS_ATTR_DIR ? 2 : 2));
        } else {
            set_color(COLOR_BLACK, COLOR_BLACK); print_spaces(TREE_WIDTH);
        }
        set_color(COLOR_DARK_GRAY, COLOR_BLACK); print_char('|');
    }
}

void draw_screen() {
    int ed_height = term_rows - 1;
    draw_tree(ed_height);
    
    int ed_w = term_cols - TREE_WIDTH - 1;
    if (is_split) {
        int w1 = ed_w / 2;
        int w2 = ed_w - w1 - 1;
        draw_editor(&eds[0], TREE_WIDTH + 1, 0, w1, ed_height, active_panel == PANEL_ED1);
        draw_editor(&eds[1], TREE_WIDTH + 1 + w1 + 1, 0, w2, ed_height, active_panel == PANEL_ED2);
    } else {
        draw_editor(&eds[0], TREE_WIDTH + 1, 0, ed_w, ed_height, active_panel == PANEL_ED1);
    }
    
    set_cursor(0, term_rows - 1);
    set_color(COLOR_BLACK, COLOR_LIGHT_GRAY);
    char footer[128];
    sprintf(footer, " F2: Save | Win+Tab: Next | Alt+Tab: Split | F1: Close | ESC: Exit | %s ", 
        (active_panel == PANEL_TREE) ? "Explorer" : (eds[active_panel-1].readonly ? "[READONLY]" : "[EDIT]"));
    printf("%s", footer);
    print_spaces(term_cols - strlen(footer) - 1);
}

int main(int argc, char** argv) {
    get_term_size(&term_cols, &term_rows);
    if(term_cols == 0) { term_cols = 80; term_rows = 25; }

    init_editor(&eds[0]);
    init_editor(&eds[1]);
    load_tree();
    
    if(argc > 1) open_file(argv[1]);

    clear_screen();

    while(1) {
        draw_screen();
        
        if (active_panel == PANEL_ED1 || active_panel == PANEL_ED2) {
            Editor *ed = &eds[active_panel - 1];
            int ed_x = TREE_WIDTH + 1;
            if (active_panel == PANEL_ED2) ed_x += (term_cols - TREE_WIDTH - 1) / 2 + 1;
            
            
            int visual_cx = 0;
            for(int i = ed->col_offset; i < ed->cx && ed->lines[ed->cy][i] != '\0'; i++) {
                if (ed->lines[ed->cy][i] == '\t') {
                    visual_cx++;
                    while(visual_cx % 4 != 0) visual_cx++;
                } else {
                    visual_cx++;
                }
            }

            int screen_x = ed_x + visual_cx + 5; 
            int screen_y = ed->cy - ed->row_offset + 1;
            
            set_cursor(screen_x, screen_y);
            set_color(COLOR_BLACK, COLOR_WHITE);
            char c = ed->lines[ed->cy][ed->cx];
            print_char((c == '\0' || c == '\n') ? ' ' : c);
            set_color(COLOR_WHITE, COLOR_BLACK);
            set_cursor(screen_x, screen_y);
        }

        unsigned int c = getc();
        
        if (c == KEY_ESC) break;
        else if (c == KEY_F1) { 
            if (is_split) {
                is_split = 0;
                active_panel = PANEL_ED1;
            } else if (active_panel == PANEL_ED1) {
                active_panel = PANEL_TREE;
            }
        }
        else if (c == 23) { 
            active_panel++;
            if (!is_split && active_panel > PANEL_ED1) active_panel = PANEL_TREE;
            if (is_split && active_panel > PANEL_ED2) active_panel = PANEL_TREE;
        }
        else if (c == 24) { 
            if (active_panel == PANEL_ED1 || active_panel == PANEL_ED2) {
                is_split = !is_split;
                if (!is_split && active_panel == PANEL_ED2) active_panel = PANEL_ED1;
            }
        }
        else if (active_panel == PANEL_TREE) { 
            if (c == KEY_UP && tree_sel > 0) tree_sel--;
            else if (c == KEY_DOWN && tree_sel < tree_cnt - 1) tree_sel++;
            else if (c == KEY_ENTER) {
                if (tree[tree_sel].type == VFS_ATTR_DIR) {
                    if (strcmp(tree[tree_sel].name, "..") == 0) {
                        for(int i = strlen(current_dir)-1; i >= 0; i--) {
                            if (current_dir[i] == '/') { current_dir[i] = '\0'; break; }
                        }
                        if (strlen(current_dir) == 0) strcpy(current_dir, "/");
                    } else {
                        if (strcmp(current_dir, "/") != 0) strcat(current_dir, "/");
                        strcat(current_dir, tree[tree_sel].name);
                    }
                    load_tree();
                } else {
                    open_file(tree[tree_sel].name);
                }
            }
            if (tree_sel < tree_offset) tree_offset = tree_sel;
            if (tree_sel >= tree_offset + term_rows - 2) tree_offset = tree_sel - (term_rows - 3);
        }
        else { 
            Editor *ed = &eds[active_panel - 1];
            int len = strlen(ed->lines[ed->cy]);
            if (ed->cx > len) ed->cx = len;
            if (c == KEY_UP && ed->cy > 0) ed->cy--;
            else if (c == KEY_DOWN && ed->cy < ed->num_lines - 1) ed->cy++;
            else if (c == KEY_LEFT && ed->cx > 0) ed->cx--;
            else if (c == KEY_RIGHT && ed->cx < strlen(ed->lines[ed->cy])) ed->cx++;
            
            
            else if (c == '\b' && !ed->readonly) { 
                if (ed->cx > 0) {
                    safe_delete_char(ed->lines[ed->cy], &ed->cx);
                } else if (ed->cy > 0) { 
                    int prev_len = strlen(ed->lines[ed->cy - 1]);
                    int cur_len = strlen(ed->lines[ed->cy]);
                    if (prev_len + cur_len < MAX_LINE_LEN - 1) {
                        strcat(ed->lines[ed->cy - 1], ed->lines[ed->cy]);
                        free(ed->lines[ed->cy]);
                        for (int i = ed->cy; i < ed->num_lines - 1; i++) {
                            ed->lines[i] = ed->lines[i + 1];
                            ed->allocs[i] = ed->allocs[i + 1];
                        }
                        ed->num_lines--;
                        ed->cy--;
                        ed->cx = prev_len;
                    }
                }
            }
            else if (c == '\n' && !ed->readonly) { 
                char *new_line = malloc(MAX_LINE_LEN);
                safe_split_line(ed->lines[ed->cy], ed->cx, new_line);
                ed->lines[ed->cy][ed->cx] = '\0';

                if (ed->num_lines >= ed->capacity) {
                    ed->capacity *= 2;
                    char **nlines = malloc(ed->capacity * sizeof(char*));
                    uint8_t *nallocs = malloc(ed->capacity * sizeof(uint8_t));
                    for(int i=0; i < ed->num_lines; i++) { nlines[i] = ed->lines[i]; nallocs[i] = ed->allocs[i]; }
                    free(ed->lines); free(ed->allocs);
                    ed->lines = nlines; ed->allocs = nallocs;
                }

                for (int i = ed->num_lines; i > ed->cy + 1; i--) {
                    ed->lines[i] = ed->lines[i - 1];
                    ed->allocs[i] = ed->allocs[i - 1];
                }
                ed->lines[ed->cy + 1] = new_line;
                ed->allocs[ed->cy + 1] = 1;
                ed->num_lines++;
                ed->cy++;
                ed->cx = 0;
            }
            else if (c == KEY_F2 && !ed->readonly) { 
                int total_size = 0;
                for(int i = 0; i < ed->num_lines; i++) total_size += strlen(ed->lines[i]) + 1;
                uint8_t *save_buf = malloc(total_size + 1);
                int pos = 0;
                for(int i = 0; i < ed->num_lines; i++) {
                    strcpy((char*)&save_buf[pos], ed->lines[i]);
                    pos += strlen(ed->lines[i]);
                    if (i < ed->num_lines - 1) save_buf[pos++] = '\n';
                }
                save_buf[pos] = '\0';
                
                char full[256];
                if (strcmp(current_dir, "/") == 0) sprintf(full, "/%s", ed->filename);
                else sprintf(full, "%s/%s", current_dir, ed->filename);
                
                write_file(full, save_buf, pos);
                free(save_buf);
            }
            else if (c == KEY_TAB && !ed->readonly){
                for (int i = 0; i < 4; i++){
                    safe_insert_char(ed->lines[ed->cy], &ed->cx, ' ');
                }
            }
            else if (c >= 32 && c <= 126 && !ed->readonly) { 
                safe_insert_char(ed->lines[ed->cy], &ed->cx, c);
            }
            

            if(ed->cy < ed->row_offset) ed->row_offset = ed->cy;
            if(ed->cy >= ed->row_offset + term_rows - 2) ed->row_offset = ed->cy - (term_rows - 3);
            if(ed->cx < ed->col_offset) ed->col_offset = ed->cx;
            
            int ed_w = term_cols - TREE_WIDTH - 1;
            if (is_split) ed_w = ed_w / 2;
            if(ed->cx >= ed->col_offset + ed_w - 6) ed->col_offset = ed->cx - (ed_w - 7);
        }
    }

    clear_screen();
    set_color(COLOR_WHITE, COLOR_BLACK);
    return 0;
}