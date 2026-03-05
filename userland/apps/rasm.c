#include "oslib.h"
#include "string_lib.h"

#define BASE_ADDRESS 0x40000000

uint8_t machine_code[1024 * 64];
int code_size = 0;
static char* next_token = NULL;

char *strtok(char *str) {
    if(str != NULL) next_token = str;
    if(next_token == NULL || *next_token == '\0') return NULL;

    while (*next_token == ' ' || *next_token == '\t' || *next_token == '\r' || *next_token == '\n') {
        next_token++;
    }

    if(*next_token == '\0') return NULL;
    char * start = next_token;

    while (*next_token != '\0' && *next_token != ' ' && *next_token != '\t' && *next_token != '\r' && *next_token != '\n') {
        next_token++;
    }

    if (*next_token != '\0') {
        *next_token = '\0';
        next_token++;
    }
    
    return start;
}

int parse_int(char *s) {
    if (s[0] == '0' && s[1] == 'x') {
        int val = 0;
        s += 2;
        while (*s) {
            char c = *s++;
            val *= 16;
            if (c >= '0' && c <= '9') val += c - '0';
            else if (c >= 'a' && c <= 'f') val += c - 'a' + 10;
        }
        return val;
    } else {
        int val = 0;
        int sign = 1;
        if (*s == '-') { sign = -1; s++; }
        while (*s >= '0' && *s <= '9') {
            val = val * 10 + (*s - '0');
            s++;
        }
        return val * sign;
    }
}

int get_reg(char *name) {
    if (strcmp(name, "eax") == 0) return 0;
    if (strcmp(name, "ecx") == 0) return 1;
    if (strcmp(name, "edx") == 0) return 2;
    if (strcmp(name, "ebx") == 0) return 3;
    if (strcmp(name, "esp") == 0) return 4;
    if (strcmp(name, "ebp") == 0) return 5;
    if (strcmp(name, "esi") == 0) return 6;
    if (strcmp(name, "edi") == 0) return 7;
    return -1;
}

int get_mem_reg(char *op) {
    if (op[0] == '[' && op[strlen(op)-1] == ']') {
        char temp[32];
        strcpy(temp, op + 1);
        temp[strlen(temp)-1] = '\0';
        return get_reg(temp);
    }
    return -1;
}

typedef struct {
    char name[32];
    int address;
} Label;

Label labels[1024];
int label_count = 0;

void add_label(char *name, int addr) {
    strcpy(labels[label_count].name, name);
    labels[label_count].address = addr;
    label_count++;
}

int get_label_addr(char *name) {
    for (int i = 0; i < label_count; i++) {
        if (strcmp(labels[i].name, name) == 0) return labels[i].address;
    }
    return -1;
}

int is_mem(char *op, int *addr) {
    if (op[0] == '[' && op[strlen(op)-1] == ']') {
        char temp[32];
        strcpy(temp, op + 1);
        temp[strlen(temp)-1] = '\0';
        
        int reg = get_reg(temp);
        if (reg >= 0) return 0; 
        
        int l_addr = get_label_addr(temp);
        if (l_addr >= 0) {
            *addr = l_addr + BASE_ADDRESS;
            return 1;
        }
        *addr = parse_int(temp);
        return 1;
    }
    return 0;
}

int get_imm(char *op) {
    int l_addr = get_label_addr(op);
    if (l_addr >= 0) return l_addr + BASE_ADDRESS;
    return parse_int(op);
}

void clean_token(char *token) {
    int len = strlen(token);
    if (len > 0 && token[len-1] == ',') {
        token[len-1] = '\0';
    }
}

void str_to_lower(char *str) {
    int in_string = 0;
    while (*str) {
        if (*str == '"' || *str == '\'') in_string = !in_string;
        if (!in_string && *str >= 'A' && *str <= 'Z') *str += 32;
        str++;
    }
}

int current_pass = 1;
int error_count = 0;

void emit8(uint8_t val) {
    if (current_pass == 2) {
        if (code_size < sizeof(machine_code)) {
            machine_code[code_size] = val;
        }
    }
    code_size++;
}

void emit32(uint32_t val) {
    emit8(val & 0xFF);
    emit8((val >> 8) & 0xFF);
    emit8((val >> 16) & 0xFF);
    emit8((val >> 24) & 0xFF);
}

void assemble_line(char *line) {
    char *comment = line;
    while (*comment) {
        if (*comment == ';') {
            *comment = '\0';
            break;
        }
        comment++;
    }

    char *inst = strtok(line);
    if (!inst) return;

    int inst_len = strlen(inst);
    if (inst[inst_len - 1] == ':') {
        if (current_pass == 1) {
            inst[inst_len - 1] = '\0';
            add_label(inst, code_size);
        }
        inst = strtok(NULL);
        if (!inst) return;
    }

    if (strcmp(inst, "nop") == 0) {
        emit8(0x90);
    } else if (strcmp(inst, "ret") == 0) {
        emit8(0xC3);
    } else if (strcmp(inst, "hlt") == 0) {
        emit8(0xF4);
    } else if (strcmp(inst, "int") == 0) {
        char *op = strtok(NULL);
        if (op) {
            emit8(0xCD);
            emit8(parse_int(op));
        } else {
            if (current_pass == 2) printf("Error: missing operand for int\n");
            error_count++;
        }
    } else if (strcmp(inst, "push") == 0 || strcmp(inst, "pop") == 0 || strcmp(inst, "inc") == 0 || strcmp(inst, "dec") == 0) {
        char *op = strtok(NULL);
        if (op) {
            clean_token(op);
            int reg = get_reg(op);
            if (reg >= 0) {
                if (strcmp(inst, "push") == 0) emit8(0x50 + reg);
                else if (strcmp(inst, "pop") == 0) emit8(0x58 + reg);
                else if (strcmp(inst, "inc") == 0) emit8(0x40 + reg);
                else if (strcmp(inst, "dec") == 0) emit8(0x48 + reg);
            } else if (strcmp(inst, "push") == 0) {
                emit8(0x68);
                emit32(get_imm(op));
            } else {
                if (current_pass == 2) printf("Error: invalid register %s\n", op);
                error_count++;
            }
        } else {
            if (current_pass == 2) printf("Error: missing operand for %s\n", inst);
            error_count++;
        }
    } else if (strcmp(inst, "jmp") == 0 || strcmp(inst, "call") == 0) {
        char *op = strtok(NULL);
        if (op) {
            if (strcmp(inst, "jmp") == 0) emit8(0xE9);
            else emit8(0xE8);
            
            if (current_pass == 2) {
                int target = get_label_addr(op);
                if (target >= 0) {
                    emit32(target - (code_size + 4));
                } else {
                    emit32(parse_int(op) - (code_size + 4));
                }
            } else {
                emit32(0);
            }
        } else {
            if (current_pass == 2) printf("Error: missing operand for %s\n", inst);
            error_count++;
        }
    } else if (inst[0] == 'j') {
        char *op = strtok(NULL);
        if (op) {
            emit8(0x0F);
            if (strcmp(inst, "je") == 0 || strcmp(inst, "jz") == 0) emit8(0x84);
            else if (strcmp(inst, "jne") == 0 || strcmp(inst, "jnz") == 0) emit8(0x85);
            else if (strcmp(inst, "jl") == 0) emit8(0x8C);
            else if (strcmp(inst, "jle") == 0) emit8(0x8D);
            else if (strcmp(inst, "jg") == 0) emit8(0x8F);
            else if (strcmp(inst, "jge") == 0) emit8(0x8E);
            else if (strcmp(inst, "jb") == 0 || strcmp(inst, "jc") == 0) emit8(0x82);
            else if (strcmp(inst, "jbe") == 0) emit8(0x86);
            else if (strcmp(inst, "ja") == 0) emit8(0x87);
            else if (strcmp(inst, "jae") == 0) emit8(0x83);
            else {
                if (current_pass == 2) printf("Error: unknown jump %s\n", inst);
                error_count++;
                emit8(0);
            }
            
            if (current_pass == 2) {
                int target = get_label_addr(op);
                if (target >= 0) {
                    emit32(target - (code_size + 4));
                } else {
                    emit32(parse_int(op) - (code_size + 4));
                }
            } else {
                emit32(0);
            }
        }
    } else if (strcmp(inst, "mov") == 0 || strcmp(inst, "add") == 0 || strcmp(inst, "sub") == 0 || strcmp(inst, "cmp") == 0 || strcmp(inst, "xor") == 0 || strcmp(inst, "and") == 0 || strcmp(inst, "or") == 0) {
        char *op1 = strtok(NULL);
        char *op2 = strtok(NULL);
        if (op1 && op2) {
            clean_token(op1);
            
            int reg1 = get_reg(op1);
            int reg2 = get_reg(op2);
            int mem_reg1 = get_mem_reg(op1);
            int mem_reg2 = get_mem_reg(op2);
            int mem_addr1, mem_addr2;
            int is_mem1 = is_mem(op1, &mem_addr1);
            int is_mem2 = is_mem(op2, &mem_addr2);
            
            if (strcmp(inst, "mov") == 0) {
                if (reg1 >= 0 && reg2 >= 0) { 
                    emit8(0x89); emit8(0xC0 | (reg2 << 3) | reg1);
                } else if (reg1 >= 0 && mem_reg2 >= 0) { 
                    if (mem_reg2 == 5) { emit8(0x8B); emit8(0x40 | (reg1 << 3) | mem_reg2); emit8(0); }
                    else if (mem_reg2 == 4) { emit8(0x8B); emit8(0x00 | (reg1 << 3) | mem_reg2); emit8(0x24); }
                    else { emit8(0x8B); emit8(0x00 | (reg1 << 3) | mem_reg2); }
                } else if (mem_reg1 >= 0 && reg2 >= 0) { 
                    if (mem_reg1 == 5) { emit8(0x89); emit8(0x40 | (reg2 << 3) | mem_reg1); emit8(0); }
                    else if (mem_reg1 == 4) { emit8(0x89); emit8(0x00 | (reg2 << 3) | mem_reg1); emit8(0x24); }
                    else { emit8(0x89); emit8(0x00 | (reg2 << 3) | mem_reg1); }
                } else if (reg1 >= 0 && is_mem2) { 
                    emit8(0x8B); emit8(0x05 | (reg1 << 3)); emit32(mem_addr2);
                } else if (is_mem1 && reg2 >= 0) { 
                    emit8(0x89); emit8(0x05 | (reg2 << 3)); emit32(mem_addr1);
                } else if (reg1 >= 0) { 
                    emit8(0xB8 + reg1); emit32(get_imm(op2));
                } else if (mem_reg1 >= 0) { 
                    if (mem_reg1 == 5) { emit8(0xC7); emit8(0x40 | mem_reg1); emit8(0); emit32(get_imm(op2)); }
                    else if (mem_reg1 == 4) { emit8(0xC7); emit8(0x00 | mem_reg1); emit8(0x24); emit32(get_imm(op2)); }
                    else { emit8(0xC7); emit8(0x00 | mem_reg1); emit32(get_imm(op2)); }
                } else if (is_mem1) {
                    emit8(0xC7); emit8(0x05); emit32(mem_addr1); emit32(get_imm(op2));
                } else {
                    if (current_pass == 2) printf("Error: invalid operands for mov\n");
                    error_count++;
                }
            } else { 
                uint8_t op_reg_reg, op_reg_imm, ext;
                if (strcmp(inst, "add") == 0) { op_reg_reg = 0x01; op_reg_imm = 0x81; ext = 0; }
                else if (strcmp(inst, "or") == 0) { op_reg_reg = 0x09; op_reg_imm = 0x81; ext = 1; }
                else if (strcmp(inst, "and") == 0) { op_reg_reg = 0x21; op_reg_imm = 0x81; ext = 4; }
                else if (strcmp(inst, "sub") == 0) { op_reg_reg = 0x29; op_reg_imm = 0x81; ext = 5; }
                else if (strcmp(inst, "xor") == 0) { op_reg_reg = 0x31; op_reg_imm = 0x81; ext = 6; }
                else if (strcmp(inst, "cmp") == 0) { op_reg_reg = 0x39; op_reg_imm = 0x81; ext = 7; }

                if (reg1 >= 0 && reg2 >= 0) {
                    emit8(op_reg_reg); emit8(0xC0 | (reg2 << 3) | reg1);
                } else if (reg1 >= 0) {
                    emit8(op_reg_imm); emit8(0xC0 | (ext << 3) | reg1); emit32(get_imm(op2));
                } else {
                    if (current_pass == 2) printf("Error: invalid operands for %s\n", inst);
                    error_count++;
                }
            }
        } else {
            if (current_pass == 2) printf("Error: missing operands for %s\n", inst);
            error_count++;
        }
    } else if (strcmp(inst, "db") == 0) {
        char *p = next_token;
        if (p) {
            while (*p) {
                if (*p == ' ' || *p == '\t' || *p == ',') {
                    p++;
                } else if (*p == '"' || *p == '\'') {
                    char quote = *p++;
                    while (*p && *p != quote) {
                        emit8(*p++);
                    }
                    if (*p == quote) p++;
                } else {
                    char temp[32];
                    int i = 0;
                    while (*p && *p != ',' && *p != ' ' && *p != '\t' && i < 31) {
                        temp[i++] = *p++;
                    }
                    temp[i] = '\0';
                    emit8(parse_int(temp));
                }
            }
            next_token = p;
        }
    } else if (strcmp(inst, "dd") == 0) {
        char *op = strtok(NULL);
        while (op) {
            clean_token(op);
            emit32(get_imm(op));
            op = strtok(NULL);
        }
    } else {
        if (current_pass == 2) printf("Error: unknown instruction %s\n", inst);
        error_count++;
    }
}

void parse_source(uint8_t *buffer) {
    char *line_start = (char*)buffer;
    while (*line_start) {
        char *line_end = line_start;
        while (*line_end != '\n' && *line_end != '\0') {
            line_end++;
        }
        
        char saved_char = *line_end;
        *line_end = '\0';

        int len = line_end - line_start;
        char *line_copy = malloc(len + 1);
        if (line_copy) {
            strcpy(line_copy, line_start);
            str_to_lower(line_copy);
            assemble_line(line_copy);
            free(line_copy);
        }

        *line_end = saved_char;

        if (saved_char == '\0') break;
        line_start = line_end + 1;
    }
}

void main(int argc, char **argv){
    if(argc < 3){
        printf("Usage: exec rasm.bin <input.asm> <output.bin>\n");
        return;
    }

    int filesize = get_file_size(argv[1]);
    if(filesize <= 0){
        printf("File is not found or empty.\n");
        return;
    }
    
    uint8_t *buffer = malloc(filesize + 1);
    if (!buffer) {
        printf("Out of memory!\n");
        return;
    }
    
    read_file(argv[1], buffer);
    buffer[filesize] = '\0';

    
    current_pass = 1;
    code_size = 0;
    label_count = 0;
    error_count = 0;
    parse_source(buffer);

    
    current_pass = 2;
    code_size = 0;
    parse_source(buffer);

    if (error_count == 0 && code_size > 0) {
        int err = write_file(argv[2], machine_code, code_size);
        if(err == 1){
            printf("Successfully compiled! %s (%d bytes)\n", argv[2], code_size);
        } else {
            printf("Error: failed to write output file! Error code: %d\n", err);
        }
    } else {
        printf("Compilation failed with %d errors.\n", error_count);
    }

    free(buffer);
}