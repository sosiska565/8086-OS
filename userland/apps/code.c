#include <oslib.h>
#include "libgui.h"

#define C_BG_EDITOR  0x001E1E1E
#define C_BG_SIDEBAR 0x00252526
#define C_BG_TAB     0x002D2D2D
#define C_BG_STATUS  0x00007ACC
#define C_DIVIDER    0x00333333
#define C_TEXT       0x00D4D4D4
#define C_KEYWORD    0x00569CD6
#define C_CONTROL    0x00C586C0
#define C_STRING     0x00CE9178
#define C_NUMBER     0x00B5CEA8
#define C_COMMENT    0x006A9955
#define C_TYPE       0x004EC9B0
#define C_CURSOR     0x00AEAFAD
#define C_SEL_BG     0x0037373D
#define C_CTX_BG     0x00252526
#define C_CTX_BORDER 0x00454545

#define MAX_LINES 1000
#define MAX_LINE_LEN 128
#define SIDEBAR_W 150
#define MAX_TABS 8

typedef struct {
    char filename[256];
    char lines[MAX_LINES][MAX_LINE_LEN];
    int num_lines;
    int cx, cy;
    int scroll_x, scroll_y;
    int is_modified;
} Tab;

Tab tabs[MAX_TABS];
int num_tabs = 0;
int active_tab = -1;

vfs_dirent_t sidebar_files[128];
int num_files = 0;
int exp_scroll_y = 0;
char cwd[256] = "/";

int ctx_active = 0;
int ctx_x = 0, ctx_y = 0;
int ctx_zone = 0; 
int ctx_target_idx = -1;

char clip_path[256] = "";
char clip_line[MAX_LINE_LEN] = "";

static int is_dragging_exp_sb = 0;
static int is_dragging_ed_sb = 0;

int is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
int is_digit(char c) { return c >= '0' && c <= '9'; }
int is_alnum(char c) { return is_alpha(c) || is_digit(c); }

void load_dir() {
    num_files = 0; vfs_dirent_t entry; int idx = 0;
    if (strcmp(cwd, "/") != 0) { strcpy(sidebar_files[num_files].name, ".."); sidebar_files[num_files].type = VFS_ATTR_DIR; num_files++; }
    while (readdir(cwd, idx++, &entry) == 1 && num_files < 128) sidebar_files[num_files++] = entry;
    if (exp_scroll_y > num_files) exp_scroll_y = 0;
}

void close_tab(int idx) {
    if (idx < 0 || idx >= num_tabs) return;
    for (int i = idx; i < num_tabs - 1; i++) tabs[i] = tabs[i + 1];
    num_tabs--;
    if (active_tab >= num_tabs) active_tab = num_tabs - 1;
}

void open_tab(char* filename) {
    for (int i = 0; i < num_tabs; i++) {
        if (strcmp(tabs[i].filename, filename) == 0) { active_tab = i; return; }
    }
    if (num_tabs >= MAX_TABS) return;

    int t = num_tabs++; active_tab = t;
    strcpy(tabs[t].filename, filename);
    tabs[t].cx = 0; tabs[t].cy = 0; tabs[t].scroll_x = 0; tabs[t].scroll_y = 0; tabs[t].is_modified = 0;

    int sz = get_file_size(filename);
    if (sz <= 0) { tabs[t].num_lines = 1; tabs[t].lines[0][0] = '\0'; return; }

    uint8_t* buf = malloc(sz + 1); read_file(filename, buf); buf[sz] = '\0';
    tabs[t].num_lines = 0; int line_idx = 0;
    for (int i = 0; i < sz; i++) {
        if (buf[i] == '\n') {
            tabs[t].lines[tabs[t].num_lines][line_idx] = '\0';
            tabs[t].num_lines++; line_idx = 0;
            if (tabs[t].num_lines >= MAX_LINES) break;
        } else if (buf[i] != '\r' && line_idx < MAX_LINE_LEN - 1) {
            tabs[t].lines[tabs[t].num_lines][line_idx++] = buf[i];
        }
    }
    if (tabs[t].num_lines < MAX_LINES) { tabs[t].lines[tabs[t].num_lines][line_idx] = '\0'; tabs[t].num_lines++; }
    free(buf);
}

void save_active_tab() {
    if (active_tab < 0) return;
    Tab *t = &tabs[active_tab];
    if (t->filename[0] == '\0') return;

    int total_sz = 0;
    for (int i = 0; i < t->num_lines; i++) total_sz += strlen(t->lines[i]) + 1; 

    uint8_t* buf = malloc(total_sz + 1); int pos = 0;
    for (int i = 0; i < t->num_lines; i++) {
        int l = strlen(t->lines[i]); memcpy(buf + pos, t->lines[i], l); pos += l; buf[pos++] = '\n';
    }
    buf[pos] = '\0'; write_file(t->filename, buf, pos);
    free(buf); t->is_modified = 0; load_dir();
}

void adjust_view(gui_window_t *win) {
    if (active_tab < 0) return;
    Tab *t = &tabs[active_tab];
    int ed_w = (win->w - SIDEBAR_W - 20) / 8;
    int ed_h = (win->h - 55) / 14; 
    
    if (t->cy < t->scroll_y) t->scroll_y = t->cy;
    if (t->cy >= t->scroll_y + ed_h) t->scroll_y = t->cy - ed_h + 1;
    if (t->cx < t->scroll_x) t->scroll_x = t->cx;
    if (t->cx >= t->scroll_x + ed_w - 2) t->scroll_x = t->cx - ed_w + 3;
}

void insert_char(char c) {
    if (active_tab < 0) return; Tab *t = &tabs[active_tab];
    int len = strlen(t->lines[t->cy]); if (len >= MAX_LINE_LEN - 1) return;
    memmove(&t->lines[t->cy][t->cx + 1], &t->lines[t->cy][t->cx], len - t->cx + 1);
    t->lines[t->cy][t->cx] = c; t->cx++; t->is_modified = 1;
}

void enter_newline() {
    if (active_tab < 0) return; Tab *t = &tabs[active_tab];
    if (t->num_lines >= MAX_LINES) return;
    for (int i = t->num_lines; i > t->cy + 1; i--) strcpy(t->lines[i], t->lines[i - 1]);
    strcpy(t->lines[t->cy + 1], &t->lines[t->cy][t->cx]);
    t->lines[t->cy][t->cx] = '\0';
    t->cy++; t->cx = 0; t->num_lines++; t->is_modified = 1;
    
    int spaces = 0;
    while(t->lines[t->cy-1][spaces] == ' ') spaces++;
    for(int i=0; i<spaces; i++) insert_char(' ');
}

void backspace_char() {
    if (active_tab < 0) return; Tab *t = &tabs[active_tab];
    if (t->cx > 0) {
        int len = strlen(t->lines[t->cy]);
        memmove(&t->lines[t->cy][t->cx - 1], &t->lines[t->cy][t->cx], len - t->cx + 1);
        t->cx--; t->is_modified = 1;
    } else if (t->cy > 0) {
        int prev_len = strlen(t->lines[t->cy - 1]);
        if (prev_len + strlen(t->lines[t->cy]) < MAX_LINE_LEN) {
            strcat(t->lines[t->cy - 1], t->lines[t->cy]);
            for (int i = t->cy; i < t->num_lines - 1; i++) strcpy(t->lines[i], t->lines[i + 1]);
            t->num_lines--; t->cy--; t->cx = prev_len; t->is_modified = 1;
        }
    }
}

uint32_t get_keyword_color(char *word) {
    const char* types[] = {"int", "char", "void", "uint32_t", "uint8_t", "size_t", "float", NULL};
    const char* kw[] = {"if", "else", "while", "for", "return", "struct", "include", "define", NULL};
    for(int i=0; types[i]; i++) if(strcmp(word, types[i]) == 0) return C_TYPE;
    for(int i=0; kw[i]; i++) if(strcmp(word, kw[i]) == 0) return C_CONTROL;
    return C_TEXT;
}

void render_editor(gui_window_t *win) {
    gui_draw_rect(win, 0, 0, SIDEBAR_W, win->h, C_BG_SIDEBAR);
    gui_draw_rect(win, SIDEBAR_W, 0, win->w - SIDEBAR_W, win->h, C_BG_EDITOR);
    gui_draw_rect(win, SIDEBAR_W, 0, win->w - SIDEBAR_W, 30, C_BG_TAB);
    gui_draw_rect(win, SIDEBAR_W, 30, win->w - SIDEBAR_W, 1, C_DIVIDER);
    
    int tab_x = SIDEBAR_W;
    for (int i = 0; i < num_tabs; i++) {
        char *fname = strrchr(tabs[i].filename, '/');
        fname = fname ? fname + 1 : tabs[i].filename;
        char title[64]; sprintf(title, " %s %s ", fname[0] ? fname : "Untitled", tabs[i].is_modified ? "●" : "  ");
        int tw = strlen(title) * 8 + 15;
        
        uint32_t bg = (i == active_tab) ? C_BG_EDITOR : C_BG_TAB;
        gui_draw_rect(win, tab_x, 0, tw, 30, bg);
        if (i == active_tab) gui_draw_rect(win, tab_x, 0, tw, 2, C_BG_STATUS); 
        gui_draw_string(win, tab_x + 5, 11, title, (i == active_tab) ? C_TEXT : 0x00888888);
        
        gui_draw_string(win, tab_x + tw - 12, 11, "x", 0x00888888); 
        
        if (win->clicked && win->my < 30 && win->mx >= tab_x && win->mx <= tab_x + tw) {
            if (win->mx >= tab_x + tw - 15) close_tab(i); else active_tab = i;
            win->clicked = 0; return;
        }
        tab_x += tw;
        gui_draw_rect(win, tab_x, 0, 1, 30, C_DIVIDER); 
    }

    gui_draw_rect(win, 0, win->h - 25, win->w, 25, C_BG_STATUS);
    if (active_tab >= 0) {
        char status[128]; sprintf(status, "Ln %d, Col %d    UTF-8    C/C++", tabs[active_tab].cy + 1, tabs[active_tab].cx + 1);
        gui_draw_string(win, win->w - strlen(status) * 8 - 15, win->h - 17, status, 0xFFFFFF);
    }
    gui_draw_string(win, 10, win->h - 17, "code", 0xFFFFFF);

    gui_draw_string(win, 15, 10, "EXPLORER", 0x00888888);
    int exp_max_vis = (win->h - 60) / 20;
    for (int i = 0; i < exp_max_vis; i++) {
        int fidx = exp_scroll_y + i;
        if (fidx >= num_files) break;
        
        int iy = 35 + i * 20;
        int hovered = (win->mx >= 0 && win->mx <= SIDEBAR_W && win->my >= iy && win->my <= iy + 20 && !ctx_active);
        
        if (hovered) {
            gui_draw_rect(win, 0, iy, SIDEBAR_W, 20, C_SEL_BG);
            if (win->clicked) {
                if (sidebar_files[fidx].type == VFS_ATTR_DIR) {
                    if (strcmp(sidebar_files[fidx].name, "..") == 0) {
                        char* ls = strrchr(cwd, '/'); if (ls && ls != cwd) *ls = '\0'; else strcpy(cwd, "/");
                    } else {
                        if (strcmp(cwd, "/") != 0) strcat(cwd, "/"); strcat(cwd, sidebar_files[fidx].name);
                    }
                    exp_scroll_y = 0; load_dir(); win->clicked = 0;
                } else {
                    char fullpath[256];
                    if (strcmp(cwd, "/") == 0) sprintf(fullpath, "/%s", sidebar_files[fidx].name);
                    else sprintf(fullpath, "%s/%s", cwd, sidebar_files[fidx].name);
                    open_tab(fullpath); win->clicked = 0;
                }
            }
        }
        uint32_t fcolor = (sidebar_files[fidx].type == VFS_ATTR_DIR) ? C_KEYWORD : C_TEXT;
        gui_draw_string(win, 15, iy + 6, sidebar_files[fidx].name, fcolor);
    }
    
    if (num_files > exp_max_vis) {
        int sh = (exp_max_vis * (win->h - 60)) / num_files; if (sh < 10) sh = 10;
        int sy = 35 + (exp_scroll_y * (win->h - 60 - sh)) / (num_files - exp_max_vis);
        uint32_t sb_col = is_dragging_exp_sb ? 0x00888888 : 0x00555555;
        gui_draw_rect(win, SIDEBAR_W - 6, sy, 5, sh, sb_col);
    }

    if (active_tab >= 0) {
        Tab *t = &tabs[active_tab];
        int ed_h = (win->h - 55) / 14;
        int max_disp_x = (win->w - SIDEBAR_W - 20) / 8;

        for (int i = 0; i < ed_h; i++) {
            int lidx = t->scroll_y + i;
            if (lidx >= t->num_lines) break;

            char* line = t->lines[lidx];
            int len = strlen(line);
            int px = SIDEBAR_W + 10;
            int py = 35 + i * 14;

            uint32_t colors[MAX_LINE_LEN];
            for(int j=0; j<len; j++) colors[j] = C_TEXT;

            int in_string = 0;
            for (int j = 0; j < len; ) {
                if (line[j] == '/' && line[j+1] == '/') { while(j < len) colors[j++] = C_COMMENT; break; }
                if (line[j] == '"') {
                    colors[j++] = C_STRING; in_string = !in_string;
                    while(j < len && line[j] != '"') colors[j++] = C_STRING;
                    if (j < len) colors[j++] = C_STRING; in_string = 0;
                    continue;
                }
                if (!in_string) {
                    if (is_alpha(line[j])) {
                        char word[64]; int wi = 0; int start = j;
                        while (j < len && is_alnum(line[j]) && wi < 63) word[wi++] = line[j++];
                        word[wi] = '\0';
                        uint32_t kw_col = get_keyword_color(word);
                        for(int k=start; k<j; k++) colors[k] = kw_col;
                        continue;
                    }
                    if (is_digit(line[j])) { while (j < len && (is_digit(line[j]) || line[j] == '.')) colors[j++] = C_NUMBER; continue; }
                }
                j++;
            }

            for (int j = t->scroll_x; j < len && (j - t->scroll_x) < max_disp_x; j++) {
                gui_draw_char(win, px, py, line[j], colors[j]);
                px += 8;
            }

            if (lidx == t->cy && (get_ticks() / 500) % 2 == 0) {
                int cx_screen = SIDEBAR_W + 10 + (t->cx - t->scroll_x) * 8;
                if (cx_screen >= SIDEBAR_W + 10 && cx_screen < win->w - 8) gui_draw_rect(win, cx_screen, py, 2, 12, C_CURSOR);
            }
        }
        
        if (t->num_lines > ed_h) {
            int r_sh = (ed_h * (win->h - 55)) / t->num_lines; if (r_sh < 10) r_sh = 10;
            int r_sy = 30 + (t->scroll_y * (win->h - 55 - r_sh)) / (t->num_lines - ed_h);
            uint32_t sb_col = is_dragging_ed_sb ? 0x00888888 : 0x00555555;
            gui_draw_rect(win, win->w - 8, r_sy, 6, r_sh, sb_col);
        }
    }

    if (ctx_active) {
        gui_draw_rect(win, ctx_x, ctx_y, 140, 85, C_CTX_BORDER);
        gui_draw_rect(win, ctx_x + 1, ctx_y + 1, 138, 83, C_CTX_BG);
        
        int menu_idx = (win->my - ctx_y) / 20;
        if (win->mx >= ctx_x && win->mx <= ctx_x + 140 && win->my >= ctx_y && win->my <= ctx_y + 80) {
            gui_draw_rect(win, ctx_x + 2, ctx_y + 2 + menu_idx*20, 136, 18, C_SEL_BG);
        }
        
        if (ctx_zone == 1) { 
            gui_draw_string(win, ctx_x + 10, ctx_y + 6, "New File", C_TEXT);
            gui_draw_string(win, ctx_x + 10, ctx_y + 26, "Copy", (ctx_target_idx >= 0) ? C_TEXT : 0x00666666);
            gui_draw_string(win, ctx_x + 10, ctx_y + 46, "Paste", clip_path[0] ? C_TEXT : 0x00666666);
            gui_draw_string(win, ctx_x + 10, ctx_y + 66, "Delete", (ctx_target_idx >= 0) ? 0x00FF453A : 0x00666666);
        } else { 
            gui_draw_string(win, ctx_x + 10, ctx_y + 6, "Save", C_TEXT);
            gui_draw_string(win, ctx_x + 10, ctx_y + 26, "Close Tab", C_TEXT);
            gui_draw_string(win, ctx_x + 10, ctx_y + 46, "Copy Line", C_TEXT);
            gui_draw_string(win, ctx_x + 10, ctx_y + 66, "Paste Line", clip_line[0] ? C_TEXT : 0x00666666);
        }

        if (win->clicked) {
            if (menu_idx >= 0 && menu_idx <= 3) {
                if (ctx_zone == 1) {
                    char target[256];
                    if (ctx_target_idx >= 0) sprintf(target, "%s/%s", strcmp(cwd,"/")==0?"":cwd, sidebar_files[ctx_target_idx].name);
                    
                    if (menu_idx == 0) { 
                        char new_file[256]; sprintf(new_file, "%s/Untitled.txt", strcmp(cwd,"/")==0?"":cwd);
                        write_file(new_file, (uint8_t*)"", 0); load_dir(); open_tab(new_file);
                    } else if (menu_idx == 1 && ctx_target_idx >= 0) { strcpy(clip_path, target); } 
                    else if (menu_idx == 2 && clip_path[0]) { 
                        int sz = get_file_size(clip_path);
                        if (sz >= 0) {
                            uint8_t* b = malloc(sz); read_file(clip_path, b);
                            char* fname = strrchr(clip_path, '/'); fname = fname ? fname + 1 : clip_path;
                            char dst[256]; sprintf(dst, "%s/%s", strcmp(cwd,"/")==0?"":cwd, fname);
                            write_file(dst, b, sz); free(b); load_dir();
                        }
                    } else if (menu_idx == 3 && ctx_target_idx >= 0) { 
                        delete_file(target); load_dir();
                        for (int i=0; i<num_tabs; i++) if (strcmp(tabs[i].filename, target) == 0) close_tab(i);
                    }
                } else if (active_tab >= 0) {
                    Tab *t = &tabs[active_tab];
                    if (menu_idx == 0) save_active_tab();
                    else if (menu_idx == 1) close_tab(active_tab);
                    else if (menu_idx == 2) strcpy(clip_line, t->lines[t->cy]);
                    else if (menu_idx == 3 && clip_line[0]) {
                        enter_newline();
                        strcpy(t->lines[t->cy], clip_line);
                        t->cx = strlen(clip_line); t->is_modified = 1;
                    }
                }
            }
            ctx_active = 0; win->clicked = 0;
        }
    }
}

int main(int argc, char** argv) {
    gui_window_t *win = gui_create_window("code", 700, 500);
    if (!win) return 1;
    gui_set_resizable(win, 1);

    if (argc > 1) {
        char* last_slash = strrchr(argv[1], '/');
        if (last_slash) {
            if (last_slash == argv[1]) strcpy(cwd, "/");
            else { int dir_len = last_slash - argv[1]; strncpy(cwd, argv[1], dir_len); cwd[dir_len] = '\0'; }
        }
        open_tab(argv[1]);
    } else {
        open_tab("/Untitled.txt");
    }
    load_dir();

    while (!win->closed) {
        gui_update(win);

        if (win->scroll_z != 0) {
            if (win->mx < SIDEBAR_W) {
                exp_scroll_y += win->scroll_z;
                if (exp_scroll_y < 0) exp_scroll_y = 0;
                int max_v = (win->h - 60) / 20;
                if (exp_scroll_y > num_files - max_v) exp_scroll_y = (num_files > max_v) ? num_files - max_v : 0;
            } else if (active_tab >= 0) {
                tabs[active_tab].scroll_y += win->scroll_z * 3;
                if (tabs[active_tab].scroll_y < 0) tabs[active_tab].scroll_y = 0;
            }
        }

        if (win->mbtn & 1) {
            if (win->clicked) {
                int exp_max_vis = (win->h - 60) / 20;
                if (num_files > exp_max_vis && win->mx >= SIDEBAR_W - 8 && win->mx <= SIDEBAR_W) {
                    is_dragging_exp_sb = 1;
                }
                
                if (active_tab >= 0) {
                    Tab *t = &tabs[active_tab];
                    int ed_h = (win->h - 55) / 14;
                    if (t->num_lines > ed_h && win->mx >= win->w - 10 && win->mx <= win->w) {
                        is_dragging_ed_sb = 1;
                    }
                }
            }
            
            if (is_dragging_exp_sb) {
                int exp_max_vis = (win->h - 60) / 20;
                if (num_files > exp_max_vis) {
                    int track_h = win->h - 60;
                    int sh = (exp_max_vis * track_h) / num_files; if (sh < 10) sh = 10;
                    int rel_y = win->my - 35 - (sh / 2);
                    if (rel_y < 0) rel_y = 0;
                    if (rel_y > track_h - sh) rel_y = track_h - sh;
                    exp_scroll_y = (rel_y * (num_files - exp_max_vis)) / (track_h - sh);
                }
            }
            
            if (is_dragging_ed_sb && active_tab >= 0) {
                Tab *t = &tabs[active_tab];
                int ed_h = (win->h - 55) / 14;
                if (t->num_lines > ed_h) {
                    int track_h = win->h - 55;
                    int r_sh = (ed_h * track_h) / t->num_lines; if (r_sh < 10) r_sh = 10;
                    int rel_y = win->my - 30 - (r_sh / 2);
                    if (rel_y < 0) rel_y = 0;
                    if (rel_y > track_h - r_sh) rel_y = track_h - r_sh;
                    t->scroll_y = (rel_y * (t->num_lines - ed_h)) / (track_h - r_sh);
                }
            }
        } else {
            is_dragging_exp_sb = 0;
            is_dragging_ed_sb = 0;
        }

        if (win->mbtn & 2) {
            if (!ctx_active) {
                ctx_active = 1; ctx_x = win->mx; ctx_y = win->my;
                if (win->mx < SIDEBAR_W) {
                    ctx_zone = 1; 
                    int fidx = exp_scroll_y + ((win->my - 35) / 20);
                    ctx_target_idx = (fidx >= 0 && fidx < num_files && strcmp(sidebar_files[fidx].name, "..") != 0) ? fidx : -1;
                } else {
                    ctx_zone = 0; 
                }
                if (ctx_y > win->h - 90) ctx_y = win->h - 90;
            }
        }

        if (win->key_code && !ctx_active && active_tab >= 0) {
            uint8_t mods = get_key_modifiers();
            char c = win->char_input;
            Tab *t = &tabs[active_tab];

            if ((mods & 2) && (c == 's' || c == 'S')) { save_active_tab(); }
            else if ((mods & 2) && (c == 'w' || c == 'W')) { close_tab(active_tab); }
            else if (win->key_code == KEY_UP) { if (t->cy > 0) { t->cy--; if (t->cx > strlen(t->lines[t->cy])) t->cx = strlen(t->lines[t->cy]); } }
            else if (win->key_code == KEY_DOWN) { if (t->cy < t->num_lines - 1) { t->cy++; if (t->cx > strlen(t->lines[t->cy])) t->cx = strlen(t->lines[t->cy]); } }
            else if (win->key_code == KEY_LEFT) { if (t->cx > 0) t->cx--; else if (t->cy > 0) { t->cy--; t->cx = strlen(t->lines[t->cy]); } }
            else if (win->key_code == KEY_RIGHT) { if (t->cx < strlen(t->lines[t->cy])) t->cx++; else if (t->cy < t->num_lines - 1) { t->cy++; t->cx = 0; } }
            else if (c == '\n' || c == '\r') { enter_newline(); }
            else if (c == '\b') { backspace_char(); }
            else if (c == '\t') { for(int i=0; i<4; i++) insert_char(' '); }
            else if (c >= 32 && c <= 126) { insert_char(c); }

            adjust_view(win);
        }

        render_editor(win);
        gui_render(win);
        yield();
    }

    gui_destroy_window(win);
    return 0;
}