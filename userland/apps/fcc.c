#include <oslib.h>


typedef struct {
    uint8_t  e_ident[16]; uint16_t e_type; uint16_t e_machine; uint32_t e_version;
    uint32_t e_entry; uint32_t e_phoff; uint32_t e_shoff; uint32_t e_flags;
    uint16_t e_ehsize; uint16_t e_phentsize; uint16_t e_phnum; uint16_t e_shentsize;
    uint16_t e_shnum; uint16_t e_shstrndx;
} Elf32_Ehdr;
typedef struct {
    uint32_t p_type; uint32_t p_offset; uint32_t p_vaddr; uint32_t p_paddr;
    uint32_t p_filesz; uint32_t p_memsz; uint32_t p_flags; uint32_t p_align;
} Elf32_Phdr;
typedef struct {
    uint32_t sh_name; uint32_t sh_type; uint32_t sh_flags; uint32_t sh_addr;
    uint32_t sh_offset; uint32_t sh_size; uint32_t sh_link; uint32_t sh_info;
    uint32_t sh_addralign; uint32_t sh_entsize;
} Elf32_Shdr;
typedef struct {
    uint32_t st_name; uint32_t st_value; uint32_t st_size; uint8_t  st_info;
    uint8_t  st_other; uint16_t st_shndx;
} Elf32_Sym;

#define ELF_MAGIC 0x464C457F
#define LOAD_ADDR 0x40000000


enum {
    T_EOF, T_NUM, T_ID, T_STR, T_CHAR_LIT, T_IF, T_ELSE, T_WHILE, T_RETURN, T_SYSCALL,
    T_INT, T_CHAR_TYPE, T_VOID, T_STRUCT, T_BREAK, T_CONTINUE,
    T_EQ, T_NE, T_LT, T_GT, T_LE, T_GE,
    T_ADD, T_SUB, T_MUL, T_DIV, T_ASSIGN,
    T_BIT_OR, T_BIT_AND, T_BIT_XOR, T_SHL, T_SHR, T_TILDE, T_NOT,
    T_LOG_OR, T_LOG_AND,
    T_SEMI, T_COMMA, T_LBRACE, T_RBRACE, T_LPAREN, T_RPAREN, T_LBRACKET, T_RBRACKET,
    T_DOT, T_ARROW, T_AMP, T_STAR
};


#define TY_INT  1
#define TY_CHAR 2
#define TY_VOID 3
#define TY_STRUCT 4

struct Type {
    int base;
    int ptr_level;
    int struct_id;
};


struct StructMember { char name[32]; struct Type type; int offset; };
struct StructDef { char name[32]; int size; struct StructMember members[32]; int mem_cnt; } structs[64];
int struct_cnt = 0;


enum { SYM_GLOBAL, SYM_LOCAL, SYM_FUNC };
struct Sym { char name[32]; int class; struct Type type; int addr; } syms[1024];
int sym_cnt = 0;


struct Macro { char name[32]; char val[128]; } macros[256];
int macro_cnt = 0;

char *src_stack[16]; int src_sp = 0; char *src;
int tok, tok_val; char tok_str[128];

uint8_t text[256000]; uint8_t data[256000];
int pc = 0, dc = 0;
int data_relocs[4096]; int data_reloc_cnt = 0;
int is_shared = 0; int local_offset = 0;
int skip_code = 0;


int break_jumps[64], break_cnt = 0;
int loop_start_addr = 0;

int is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_'); }
int is_num(char c) { return (c >= '0' && c <= '9'); }
int is_space_c(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

int find_sym(char *name) { for(int i = sym_cnt - 1; i >= 0; i--) if(strcmp(syms[i].name, name) == 0) return i; return -1; }
int add_sym(char *name, int class, struct Type t, int addr) { strcpy(syms[sym_cnt].name, name); syms[sym_cnt].class = class; syms[sym_cnt].type = t; syms[sym_cnt].addr = addr; return sym_cnt++; }
int find_struct(char *name) { for(int i=0; i<struct_cnt; i++) if(strcmp(structs[i].name, name)==0) return i; return -1; }

int type_size(struct Type t) {
    if (t.ptr_level > 0) return 4;
    if (t.base == TY_CHAR) return 1;
    if (t.base == TY_STRUCT) return structs[t.struct_id].size;
    return 4;
}

void next() {
    while(1) {
        while(is_space_c(*src)) src++;
        if (!*src) { if (src_sp > 0) { src = src_stack[--src_sp]; continue; } tok = T_EOF; return; }
        if (*src == '/' && *(src+1) == '/') { while(*src && *src != '\n') src++; continue; }
        if (*src == '/' && *(src+1) == '*') {
            src += 2; while(*src && !(*src == '*' && *(src+1) == '/')) src++;
            if(*src) src += 2; continue;
        }

        if (*src == '#') {
            src++; char cmd[32]; int i = 0; while(is_alpha(*src)) cmd[i++] = *src++; cmd[i] = '\0';
            if (strcmp(cmd, "include") == 0) {
                while(*src == ' ' || *src == '"' || *src == '<') src++;
                char inc_file[64]; i = 0;
                while(*src && *src != '"' && *src != '>' && *src != '\n') inc_file[i++] = *src++; inc_file[i] = '\0';
                if(*src == '"' || *src == '>') src++;
                if (!skip_code) {
                    int fsize = get_file_size(inc_file);
                    if (fsize > 0 && src_sp < 15) {
                        char *new_src = malloc(fsize + 1); read_file(inc_file, (uint8_t*)new_src); new_src[fsize] = '\0';
                        src_stack[src_sp++] = src; src = new_src;
                    } else { printf("FCC Error: Cannot include '%s'\n", inc_file); exit(); }
                } continue;
            }
            else if (strcmp(cmd, "define") == 0) {
                while(*src == ' ') src++; char mname[32]; i = 0;
                while(is_alpha(*src) || is_num(*src)) mname[i++] = *src++; mname[i] = '\0';
                while(*src == ' ') src++; char mval[128]; i = 0;
                while(*src && *src != '\n') mval[i++] = *src++; mval[i] = '\0';
                if (!skip_code && macro_cnt < 256) { strcpy(macros[macro_cnt].name, mname); strcpy(macros[macro_cnt].val, mval); macro_cnt++; }
                continue;
            }
            else if (strcmp(cmd, "ifdef") == 0 || strcmp(cmd, "ifndef") == 0) {
                while(*src == ' ') src++; char mname[32]; i = 0;
                while(is_alpha(*src) || is_num(*src)) mname[i++] = *src++; mname[i] = '\0';
                int found = 0; for(int m=0; m<macro_cnt; m++) if(strcmp(macros[m].name, mname)==0) { found=1; break; }
                if (strcmp(cmd, "ifdef") == 0) skip_code = !found; else skip_code = found;
                continue;
            }
            else if (strcmp(cmd, "else") == 0) { skip_code = !skip_code; continue; }
            else if (strcmp(cmd, "endif") == 0) { skip_code = 0; continue; }
        }
        if (skip_code) { while(*src && *src != '\n') src++; continue; }
        break;
    }

    if (*src >= '0' && *src <= '9') {
        tok_val = 0;
        if (*src == '0' && *(src+1) == 'x') { src += 2;
            while(1) {
                if (*src >= '0' && *src <= '9') tok_val = tok_val * 16 + (*src++ - '0');
                else if (*src >= 'A' && *src <= 'F') tok_val = tok_val * 16 + (*src++ - 'A' + 10);
                else if (*src >= 'a' && *src <= 'f') tok_val = tok_val * 16 + (*src++ - 'a' + 10); else break;
            }
        } else { while(*src >= '0' && *src <= '9') tok_val = tok_val * 10 + (*src++ - '0'); }
        tok = T_NUM; return;
    }

    if (is_alpha(*src)) {
        int i = 0; while(is_alpha(*src) || is_num(*src)) tok_str[i++] = *src++; tok_str[i] = '\0';
        for(int m=0; m<macro_cnt; m++) {
            if(strcmp(macros[m].name, tok_str) == 0 && src_sp < 15) { src_stack[src_sp++] = src; src = macros[m].val; next(); return; }
        }
        if (strcmp(tok_str, "int") == 0) tok = T_INT; else if (strcmp(tok_str, "char") == 0) tok = T_CHAR_TYPE;
        else if (strcmp(tok_str, "void") == 0) tok = T_VOID; else if (strcmp(tok_str, "struct") == 0) tok = T_STRUCT;
        else if (strcmp(tok_str, "if") == 0) tok = T_IF; else if (strcmp(tok_str, "else") == 0) tok = T_ELSE;
        else if (strcmp(tok_str, "while") == 0) tok = T_WHILE; else if (strcmp(tok_str, "break") == 0) tok = T_BREAK;
        else if (strcmp(tok_str, "continue") == 0) tok = T_CONTINUE; else if (strcmp(tok_str, "return") == 0) tok = T_RETURN;
        else if (strcmp(tok_str, "syscall") == 0) tok = T_SYSCALL; else tok = T_ID;
        return;
    }

    if (*src == '"') {
        src++; int i = 0;
        while(*src && *src != '"') {
            if (*src == '\\' && *(src+1) == 'n') { tok_str[i++] = '\n'; src += 2; }
            else if (*src == '\\' && *(src+1) == 't') { tok_str[i++] = '\t'; src += 2; }
            else if (*src == '\\' && *(src+1) == '0') { tok_str[i++] = '\0'; src += 2; }
            else if (*src == '\\' && *(src+1) == '"') { tok_str[i++] = '"'; src += 2; }
            else if (*src == '\\' && *(src+1) == '\\') { tok_str[i++] = '\\'; src += 2; }
            else tok_str[i++] = *src++;
        } tok_str[i] = '\0'; if(*src == '"') src++; tok = T_STR; return;
    }
    if (*src == '\'') {
        src++; if (*src == '\\' && *(src+1) == 'n') { tok_val = '\n'; src += 2; }
        else if (*src == '\\' && *(src+1) == '0') { tok_val = '\0'; src += 2; }
        else if (*src == '\\' && *(src+1) == 't') { tok_val = '\t'; src += 2; }
        else tok_val = *src++;
        if (*src == '\'') src++; tok = T_NUM; return;
    }

    if (*src == '=' && *(src+1) == '=') { src+=2; tok = T_EQ; return; }
    if (*src == '!' && *(src+1) == '=') { src+=2; tok = T_NE; return; }
    if (*src == '<' && *(src+1) == '=') { src+=2; tok = T_LE; return; }
    if (*src == '>' && *(src+1) == '=') { src+=2; tok = T_GE; return; }
    if (*src == '<' && *(src+1) == '<') { src+=2; tok = T_SHL; return; }
    if (*src == '>' && *(src+1) == '>') { src+=2; tok = T_SHR; return; }
    if (*src == '&' && *(src+1) == '&') { src+=2; tok = T_LOG_AND; return; }
    if (*src == '|' && *(src+1) == '|') { src+=2; tok = T_LOG_OR; return; }
    if (*src == '-' && *(src+1) == '>') { src+=2; tok = T_ARROW; return; }

    char c = *src++;
    if (c == '=') tok = T_ASSIGN; else if (c == '<') tok = T_LT;  else if (c == '>') tok = T_GT;
    else if (c == '+') tok = T_ADD; else if (c == '-') tok = T_SUB; else if (c == '*') tok = T_STAR;
    else if (c == '/') tok = T_DIV; else if (c == '&') tok = T_AMP; else if (c == '|') tok = T_BIT_OR;
    else if (c == '^') tok = T_BIT_XOR; else if (c == '~') tok = T_TILDE; else if (c == '!') tok = T_NOT;
    else if (c == ';') tok = T_SEMI; else if (c == '{') tok = T_LBRACE; else if (c == '}') tok = T_RBRACE;
    else if (c == '(') tok = T_LPAREN; else if (c == ')') tok = T_RPAREN;
    else if (c == '[') tok = T_LBRACKET; else if (c == ']') tok = T_RBRACKET;
    else if (c == ',') tok = T_COMMA; else if (c == '.') tok = T_DOT;
    else { printf("Syntax error: '%c'\n", c); exit(); }
}
void match(int expected) { if (tok == expected) next(); else { printf("Parse error: Expected %d, got %d\n", expected, tok); exit(); } }

void emit(uint8_t b) { text[pc++] = b; }
void emit32(uint32_t w) { text[pc++] = w & 0xFF; text[pc++] = (w >> 8) & 0xFF; text[pc++] = (w >> 16) & 0xFF; text[pc++] = (w >> 24) & 0xFF; }

void emit_load(struct Type t) {
    if (t.base == TY_STRUCT && t.ptr_level == 0) return;
    if (t.ptr_level > 0 || t.base == TY_INT) { emit(0x8B); emit(0x00); }
    else if (t.base == TY_CHAR) { emit(0x0F); emit(0xB6); emit(0x00); }
}
void emit_store(struct Type t) {
    if (t.base == TY_STRUCT && t.ptr_level == 0) return;
    if (t.ptr_level > 0 || t.base == TY_INT) { emit(0x89); emit(0x01); }
    else if (t.base == TY_CHAR) { emit(0x88); emit(0x01); }
}

struct Type parse_expr(int is_lvalue);

struct Type parse_primary(int is_lvalue) {
    struct Type t = {TY_INT, 0, 0};
    if (tok == T_NUM) { emit(0xB8); emit32(tok_val); next(); }
    else if (tok == T_STR) {
        
        int data_offset = dc; char *s = tok_str; while(*s) data[dc++] = *s++; data[dc++] = '\0';
        emit(0xB8); data_relocs[data_reloc_cnt++] = pc; emit32(data_offset); next();
        t.base = TY_CHAR; t.ptr_level = 1;
    }
    else if (tok == T_ID) {
        int id = find_sym(tok_str);
        if (id == -1) id = add_sym(tok_str, SYM_FUNC, (struct Type){TY_INT, 0, 0}, 0);
        t = syms[id].type; char name[64]; strcpy(name, tok_str); next();

        if (tok == T_LPAREN) {
            next(); int args = 0;
            while(tok != T_RPAREN) { parse_expr(0); emit(0x50); args++; if (tok == T_COMMA) next(); }
            match(T_RPAREN); emit(0xE8);
            if (syms[id].addr == 0) emit32(id); else emit32(syms[id].addr - (pc + 4));
            if (args > 0) { emit(0x83); emit(0xC4); emit(args * 4); }
            t.base = TY_INT; t.ptr_level = 0;
        } else {
            if (syms[id].class == SYM_LOCAL) { emit(0x8D); emit(0x85); emit32(syms[id].addr); }
            else { emit(0xB8); data_relocs[data_reloc_cnt++] = pc; emit32(syms[id].addr); }
            if (!is_lvalue) emit_load(t);
        }
    }
    else if (tok == T_LPAREN) { next(); t = parse_expr(0); match(T_RPAREN); }
    return t;
}

struct Type parse_postfix(int is_lvalue) {
    struct Type t = parse_primary(is_lvalue);
    while (tok == T_LBRACKET || tok == T_DOT || tok == T_ARROW) {
        if (tok == T_LBRACKET) {
            next(); emit(0x50); parse_expr(0); match(T_RBRACKET);
            t.ptr_level--; int sz = type_size(t);
            if (sz > 1) { emit(0x89); emit(0xC1); emit(0xB8); emit32(sz); emit(0xF7); emit(0xE1); }
            emit(0x59); emit(0x01); emit(0xC8);
            if (!is_lvalue) emit_load(t);
        } else if (tok == T_DOT || tok == T_ARROW) {
            int is_ptr = (tok == T_ARROW); next();
            char mname[32]; strcpy(mname, tok_str); match(T_ID);
            if (t.base != TY_STRUCT) { printf("Error: Member access on non-struct\n"); exit(); }
            if (is_ptr && !is_lvalue) { emit(0x8B); emit(0x00); }

            struct StructDef *sd = &structs[t.struct_id]; int offset = -1;
            for(int i=0; i<sd->mem_cnt; i++) {
                if(strcmp(sd->members[i].name, mname)==0) { offset = sd->members[i].offset; t = sd->members[i].type; break; }
            }
            if (offset == -1) { printf("Error: Struct member '%s' not found\n", mname); exit(); }
            if (offset > 0) { emit(0x05); emit32(offset); }
            if (!is_lvalue) emit_load(t);
        }
    }
    return t;
}

struct Type parse_unary(int is_lvalue) {
    if (tok == T_STAR) { next(); struct Type t = parse_unary(0); t.ptr_level--; if (!is_lvalue) emit_load(t); return t; }
    else if (tok == T_AMP) { next(); struct Type t = parse_unary(1); t.ptr_level++; return t; }
    else if (tok == T_NOT) { next(); struct Type t = parse_unary(0); emit(0x85); emit(0xC0); emit(0x0F); emit(0x94); emit(0xC0); emit(0x0F); emit(0xB6); emit(0xC0); t.base=TY_INT; t.ptr_level=0; return t; }
    else if (tok == T_TILDE) { next(); struct Type t = parse_unary(0); emit(0xF7); emit(0xD0); return t; }
    return parse_postfix(is_lvalue);
}

struct Type parse_mult() {
    struct Type t = parse_unary(0);
    while (tok == T_STAR || tok == T_DIV) {
        int op = tok; next(); emit(0x50); parse_unary(0); emit(0x59);
        if (op == T_STAR) { emit(0xF7); emit(0xE1); } else { emit(0x91); emit(0x99); emit(0xF7); emit(0xF9); }
    } return t;
}
struct Type parse_add() {
    struct Type t = parse_mult();
    while (tok == T_ADD || tok == T_SUB) {
        int op = tok; next(); emit(0x50); struct Type tr = parse_mult(); emit(0x59);
        if (t.ptr_level > 0 && tr.ptr_level == 0) {
            int sz = type_size((struct Type){t.base, t.ptr_level-1, t.struct_id});
            if (sz > 1) { emit(0x51); emit(0x89); emit(0xC1); emit(0xB8); emit32(sz); emit(0xF7); emit(0xE1); emit(0x59); }
        }
        if (op == T_ADD) { emit(0x01); emit(0xC8); } else { emit(0x29); emit(0xC1); emit(0x89); emit(0xC8); }
    } return t;
}
struct Type parse_shift() {
    struct Type t = parse_add();
    while(tok == T_SHL || tok == T_SHR) {
        int op = tok; next(); emit(0x50); parse_add(); emit(0x89); emit(0xC1); emit(0x58);
        if(op == T_SHL) { emit(0xD3); emit(0xE0); } else { emit(0xD3); emit(0xE8); }
    } return t;
}
struct Type parse_cmp() {
    struct Type t = parse_shift();
    if (tok >= T_EQ && tok <= T_GE) {
        int op = tok; next(); emit(0x50); parse_shift(); emit(0x59);
        emit(0x39); emit(0xC1); emit(0xB8); emit32(0);
        if (op == T_EQ) { emit(0x0F); emit(0x94); emit(0xC0); } if (op == T_NE) { emit(0x0F); emit(0x95); emit(0xC0); }
        if (op == T_LT) { emit(0x0F); emit(0x9C); emit(0xC0); } if (op == T_GT) { emit(0x0F); emit(0x9F); emit(0xC0); }
        if (op == T_LE) { emit(0x0F); emit(0x9E); emit(0xC0); } if (op == T_GE) { emit(0x0F); emit(0x9D); emit(0xC0); }
        t.base = TY_INT; t.ptr_level = 0;
    } return t;
}
struct Type parse_bit_and() {
    struct Type t = parse_cmp(); while(tok == T_AMP) { next(); emit(0x50); parse_cmp(); emit(0x59); emit(0x21); emit(0xC8); } return t;
}
struct Type parse_bit_xor() {
    struct Type t = parse_bit_and(); while(tok == T_BIT_XOR) { next(); emit(0x50); parse_bit_and(); emit(0x59); emit(0x31); emit(0xC8); } return t;
}
struct Type parse_bit_or() {
    struct Type t = parse_bit_xor(); while(tok == T_BIT_OR) { next(); emit(0x50); parse_bit_xor(); emit(0x59); emit(0x09); emit(0xC8); } return t;
}
struct Type parse_log_and() {
    struct Type t = parse_bit_or();
    while (tok == T_LOG_AND) {
        next(); emit(0x85); emit(0xC0); emit(0x0F); emit(0x84); int jmp = pc; emit32(0);
        parse_bit_or(); emit(0x85); emit(0xC0); emit(0x0F); emit(0x95); emit(0xC0); emit(0x0F); emit(0xB6); emit(0xC0);
        *(uint32_t*)&text[jmp] = pc - (jmp + 4);
    } return t;
}
struct Type parse_log_or() {
    struct Type t = parse_log_and();
    while (tok == T_LOG_OR) {
        next(); emit(0x85); emit(0xC0); emit(0x0F); emit(0x85); int jmp = pc; emit32(0);
        parse_log_and(); emit(0x85); emit(0xC0); emit(0x0F); emit(0x95); emit(0xC0); emit(0x0F); emit(0xB6); emit(0xC0);
        *(uint32_t*)&text[jmp] = pc - (jmp + 4);
    } return t;
}



struct Type parse_assign() {
    char *saved_src = src;
    int saved_pc = pc;
    int saved_tok = tok;
    int saved_dc = dc;                      
    int saved_reloc = data_reloc_cnt;       
    char saved_str[128]; strcpy(saved_str, tok_str);

    struct Type t = parse_unary(1);
    if (tok == T_ASSIGN) {
        next();
        emit(0x50);
        parse_assign();
        emit(0x59); emit_store(t);
        return t;
    } else {
        
        src = saved_src;
        tok = saved_tok;
        strcpy(tok_str, saved_str);
        pc = saved_pc;
        dc = saved_dc;                      
        data_reloc_cnt = saved_reloc;       
        return parse_log_or();
    }
}


struct Type parse_expr(int is_lvalue) {
    return parse_assign();
}

struct Type parse_type() {
    struct Type t = {TY_INT, 0, 0};
    if (tok == T_INT) { t.base = TY_INT; next(); }
    else if (tok == T_CHAR_TYPE) { t.base = TY_CHAR; next(); }
    else if (tok == T_VOID) { t.base = TY_VOID; next(); }
    else if (tok == T_STRUCT) {
        t.base = TY_STRUCT; next(); char sname[32]; strcpy(sname, tok_str); match(T_ID);
        t.struct_id = find_struct(sname);
        if(t.struct_id == -1) { printf("Undefined struct %s\n", sname); exit(); }
    }
    while(tok == T_STAR) { t.ptr_level++; next(); }
    return t;
}

void parse_stmt() {
    if (tok == T_INT || tok == T_CHAR_TYPE || tok == T_VOID || tok == T_STRUCT) {
        struct Type t = parse_type();
        char vname[64]; strcpy(vname, tok_str); match(T_ID);

        int arr_size = 1;
        if (tok == T_LBRACKET) { next(); arr_size = tok_val; match(T_NUM); match(T_RBRACKET); t.ptr_level++; }

        local_offset -= (type_size((struct Type){t.base, 0, t.struct_id}) * arr_size);
        add_sym(vname, SYM_LOCAL, t, local_offset);

        if (tok == T_ASSIGN) {
            next();
            
            if (t.base == TY_CHAR && t.ptr_level == 1 && tok == T_STR) {
                int data_offset = dc; char *s = tok_str;
                while(*s) data[dc++] = *s++; data[dc++] = '\0';
                emit(0xB8); data_relocs[data_reloc_cnt++] = pc; emit32(data_offset); next();
            } else {
                parse_assign();
            }
            emit(0x8D); emit(0x8D); emit32(local_offset);
            emit_store(t);
        }
        match(T_SEMI); return;
    }

    if (tok == T_IF) {
        next(); match(T_LPAREN); parse_assign(); match(T_RPAREN);
        emit(0x85); emit(0xC0); emit(0x0F); emit(0x84); int jmp_addr = pc; emit32(0);
        parse_stmt();
        if (tok == T_ELSE) {
            next(); emit(0xE9); int jmp_end = pc; emit32(0);
            *(uint32_t*)&text[jmp_addr] = pc - (jmp_addr + 4); parse_stmt();
            *(uint32_t*)&text[jmp_end] = pc - (jmp_end + 4);
        } else { *(uint32_t*)&text[jmp_addr] = pc - (jmp_addr + 4); }
        return;
    }

    if (tok == T_WHILE) {
        int saved_loop_start = loop_start_addr; int saved_break_cnt = break_cnt;
        loop_start_addr = pc; break_cnt = 0;

        next(); match(T_LPAREN); parse_assign(); match(T_RPAREN);
        emit(0x85); emit(0xC0); emit(0x0F); emit(0x84); int jmp_end = pc; emit32(0);
        parse_stmt();
        emit(0xE9); emit32(loop_start_addr - (pc + 4));
        *(uint32_t*)&text[jmp_end] = pc - (jmp_end + 4);

        for(int i=0; i<break_cnt; i++) *(uint32_t*)&text[break_jumps[i]] = pc - (break_jumps[i] + 4);

        loop_start_addr = saved_loop_start; break_cnt = saved_break_cnt;
        return;
    }

    if (tok == T_BREAK) { next(); match(T_SEMI); emit(0xE9); break_jumps[break_cnt++] = pc; emit32(0); return; }
    if (tok == T_CONTINUE) { next(); match(T_SEMI); emit(0xE9); emit32(loop_start_addr - (pc + 4)); return; }

    if (tok == T_RETURN) {
        next(); if (tok != T_SEMI) parse_assign(); match(T_SEMI);
        emit(0xC9); emit(0xC3); return;
    }

    if (tok == T_LBRACE) {
        next(); int saved_sym_cnt = sym_cnt;
        while(tok != T_RBRACE && tok != T_EOF) parse_stmt();
        sym_cnt = saved_sym_cnt; match(T_RBRACE); return;
    }

    
    
    
    if (tok == T_SYSCALL) {
        next(); match(T_LPAREN);
        
        
        
        
        parse_assign(); emit(0x50); if(tok == T_COMMA) next(); 
        parse_assign(); emit(0x50); if(tok == T_COMMA) next(); 
        parse_assign(); emit(0x50); if(tok == T_COMMA) next(); 
        parse_assign(); emit(0x50);                            
        
        emit(0x5A);             
        emit(0x59);             
        emit(0x5B);             
        emit(0x58);             
        emit(0xCD); emit(0x80); 
        
        match(T_RPAREN); match(T_SEMI); return;
    }

    parse_assign(); match(T_SEMI);
}

void parse_global() {
    if (tok == T_EOF) return;

    if (tok == T_STRUCT) {
        next(); char sname[32]; strcpy(sname, tok_str); match(T_ID);
        if (tok == T_LBRACE) {
            next();
            struct StructDef *sd = &structs[struct_cnt++]; strcpy(sd->name, sname);
            sd->mem_cnt = 0; sd->size = 0;
            while(tok != T_RBRACE) {
                struct Type mt = parse_type();
                strcpy(sd->members[sd->mem_cnt].name, tok_str); match(T_ID); match(T_SEMI);
                sd->members[sd->mem_cnt].type = mt;
                sd->members[sd->mem_cnt].offset = sd->size;
                sd->size += type_size(mt);
                sd->mem_cnt++;
            }
            match(T_RBRACE); match(T_SEMI); return;
        } else {
            src -= strlen(sname); tok = T_STRUCT;
        }
    }

    struct Type t = parse_type();
    char name[64]; strcpy(name, tok_str); match(T_ID);

    if (tok == T_LPAREN) {
        int id = find_sym(name);
        if (id == -1) id = add_sym(name, SYM_FUNC, t, pc); else syms[id].addr = pc;

        next(); int arg_offset = 8; int saved_sym_cnt = sym_cnt; local_offset = 0;
        while(tok != T_RPAREN) {
            struct Type at = parse_type();
            add_sym(tok_str, SYM_LOCAL, at, arg_offset); arg_offset += 4; match(T_ID);
            if (tok == T_COMMA) next();
        } match(T_RPAREN);
        if (tok == T_SEMI) { next(); return; }

        emit(0x55); emit(0x89); emit(0xE5); emit(0x81); emit(0xEC); emit32(16384);
        parse_stmt(); emit(0xC9); emit(0xC3); sym_cnt = saved_sym_cnt;
    } else {
        int arr_size = 1;
        if (tok == T_LBRACKET) { next(); arr_size = tok_val; match(T_NUM); match(T_RBRACKET); t.ptr_level++; }

        int g_offset = dc; add_sym(name, SYM_GLOBAL, t, g_offset);
        dc += (type_size((struct Type){t.base, 0, t.struct_id}) * arr_size);

        if (tok == T_ASSIGN) {
            next();
            if (t.base == TY_CHAR && t.ptr_level == 1 && tok == T_STR) {
                
                
                
                
                int str_off = dc; char *s = tok_str; while(*s) data[dc++] = *s++; data[dc++] = 0;
                *(uint32_t*)&data[g_offset] = str_off;
                data_relocs[data_reloc_cnt++] = g_offset | 0x80000000; next();
            } else { *(uint32_t*)&data[g_offset] = tok_val; next(); }
        } match(T_SEMI);
    }
}

int main(int argc, char** argv) {
    if (argc < 3) { printf("fcc - C Compiler\nUsage: fcc [-shared] <in.c> <out.elf>\n"); return 1; }
    int in_idx = 1, out_idx = 2;
    if (strcmp(argv[1], "-shared") == 0) { is_shared = 1; in_idx = 2; out_idx = 3; }

    int size = get_file_size(argv[in_idx]);
    if (size <= 0) { printf("fcc: source error\n"); return 1; }

    uint8_t *script_buffer = (uint8_t*)malloc(size + 1);
    read_file(argv[in_idx], script_buffer); script_buffer[size] = '\0';
    src = (char*)script_buffer;

    printf("Compiling %s...\n", argv[in_idx]);

    
    if (!is_shared) { emit(0xE8); emit32(0); emit(0xB8); emit32(1); emit(0xCD); emit(0x80); }

    next(); while (tok != T_EOF) parse_global();

    if (!is_shared) {
        int main_id = find_sym("main");
        if (main_id == -1) { printf("Error: main() not found!\n"); return 1; }
        *(uint32_t*)&text[1] = syms[main_id].addr - 5;
    }

    uint32_t base_load = is_shared ? 0 : LOAD_ADDR;

    
    
    
    for(int i = 0; i < data_reloc_cnt; i++) {
        if (data_relocs[i] & 0x80000000) {
            int p = data_relocs[i] & 0x7FFFFFFF;
            *(uint32_t*)&data[p] = base_load + pc + *(uint32_t*)&data[p];
        } else {
            int p = data_relocs[i];
            *(uint32_t*)&text[p] = base_load + pc + *(uint32_t*)&text[p];
        }
    }

    
    for(int i = 0; i < pc; i++) {
        if (text[i] == 0xE8) {
            uint32_t id = *(uint32_t*)&text[i+1];
            if (id < 1024 && syms[id].class == SYM_FUNC) {
                if (syms[id].addr == 0) { printf("Link Error: Undefined '%s'\n", syms[id].name); return 1; }
                *(uint32_t*)&text[i+1] = syms[id].addr - (i + 5);
            }
            i += 4; 
        }
    }

    char strtab[4096]; int strtab_len = 1; strtab[0] = '\0';
    Elf32_Sym symtab_data[1024]; int symtab_count = 1; memset(&symtab_data[0], 0, sizeof(Elf32_Sym));
    for (int i = 0; i < sym_cnt; i++) {
        if (syms[i].class == SYM_FUNC) {
            symtab_data[symtab_count].st_name = strtab_len; strcpy(&strtab[strtab_len], syms[i].name); strtab_len += strlen(syms[i].name) + 1;
            symtab_data[symtab_count].st_value = syms[i].addr + (is_shared ? 0 : base_load);
            symtab_data[symtab_count].st_info = (1 << 4) | 2; symtab_data[symtab_count].st_shndx = 1; symtab_count++;
        }
    }

    char shstrtab[] = "\0.text\0.data\0.shstrtab\0.symtab\0.strtab\0"; int shstrtab_len = sizeof(shstrtab);
    int text_off = sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr); int data_off = text_off + pc;
    int shstr_off = data_off + dc; int strtab_off = shstr_off + shstrtab_len;
    int symtab_off = strtab_off + strtab_len; int shdrs_off = symtab_off + (symtab_count * sizeof(Elf32_Sym));

    Elf32_Ehdr ehdr; memset(&ehdr, 0, sizeof(Elf32_Ehdr));
    ehdr.e_ident[0] = 0x7F; ehdr.e_ident[1] = 'E'; ehdr.e_ident[2] = 'L'; ehdr.e_ident[3] = 'F';
    ehdr.e_ident[4] = 1; ehdr.e_ident[5] = 1; ehdr.e_ident[6] = 1;
    ehdr.e_type = is_shared ? 3 : 2; ehdr.e_machine = 3; ehdr.e_version = 1;
    ehdr.e_entry = is_shared ? 0 : LOAD_ADDR;
    ehdr.e_phoff = sizeof(Elf32_Ehdr); ehdr.e_shoff = shdrs_off;
    ehdr.e_ehsize = sizeof(Elf32_Ehdr); ehdr.e_phentsize = sizeof(Elf32_Phdr); ehdr.e_phnum = 1;
    ehdr.e_shentsize = sizeof(Elf32_Shdr); ehdr.e_shnum = 6; ehdr.e_shstrndx = 3;

    Elf32_Phdr phdr; memset(&phdr, 0, sizeof(Elf32_Phdr));
    phdr.p_type = 1; phdr.p_offset = text_off;
    phdr.p_vaddr = is_shared ? 0 : LOAD_ADDR; phdr.p_paddr = phdr.p_vaddr;
    phdr.p_filesz = pc + dc; phdr.p_memsz = pc + dc; phdr.p_flags = 7; phdr.p_align = 0x1000;

    Elf32_Shdr shdrs[6]; memset(shdrs, 0, sizeof(shdrs));
    shdrs[1].sh_name = 1; shdrs[1].sh_type = 1; shdrs[1].sh_flags = 6;
    shdrs[1].sh_addr = phdr.p_vaddr; shdrs[1].sh_offset = text_off; shdrs[1].sh_size = pc;
    shdrs[2].sh_name = 7; shdrs[2].sh_type = 1; shdrs[2].sh_flags = 3;
    shdrs[2].sh_addr = phdr.p_vaddr + pc; shdrs[2].sh_offset = data_off; shdrs[2].sh_size = dc;
    shdrs[3].sh_name = 13; shdrs[3].sh_type = 3; shdrs[3].sh_offset = shstr_off; shdrs[3].sh_size = shstrtab_len;
    shdrs[4].sh_name = 31; shdrs[4].sh_type = 3; shdrs[4].sh_offset = strtab_off; shdrs[4].sh_size = strtab_len;
    shdrs[5].sh_name = 23; shdrs[5].sh_type = 2; shdrs[5].sh_offset = symtab_off;
    shdrs[5].sh_size = symtab_count * sizeof(Elf32_Sym); shdrs[5].sh_link = 4; shdrs[5].sh_entsize = sizeof(Elf32_Sym);

    int total_elf_size = shdrs_off + (6 * sizeof(Elf32_Shdr));
    uint8_t *bin = malloc(total_elf_size); memset(bin, 0, total_elf_size);

    memcpy(bin, &ehdr, sizeof(Elf32_Ehdr)); memcpy(bin + ehdr.e_phoff, &phdr, sizeof(Elf32_Phdr));
    memcpy(bin + text_off, text, pc); memcpy(bin + data_off, data, dc);
    memcpy(bin + shstr_off, shstrtab, shstrtab_len); memcpy(bin + strtab_off, strtab, strtab_len);
    memcpy(bin + symtab_off, symtab_data, symtab_count * sizeof(Elf32_Sym));
    memcpy(bin + shdrs_off, shdrs, 6 * sizeof(Elf32_Shdr));

    write_file(argv[out_idx], bin, total_elf_size);
    printf("Successfully compiled! (%d bytes)\n", total_elf_size);
    free(script_buffer); free(bin);
    return 0;
}