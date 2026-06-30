/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/apps/browser.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include <oslib.h>
#include "libgui.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

#define HTTP_BUF_SIZE (64 * 1024)
#define MAX_ELEMENTS  2048
#define MAX_HISTORY   16
#define MAX_TABS      8

#define ELEM_TEXT    0
#define ELEM_LINK    1
#define ELEM_NEWLINE 2
#define ELEM_HEADING 3

uint32_t c_bg = 0x001E1E1E;
uint32_t c_ui = 0x002D2D2D;
uint32_t c_ui_hover = 0x003D3D3D;
uint32_t c_fg = 0x00D4D4D4;
uint32_t c_accent = 0x000A84FF;
uint32_t c_border = 0x00111111;

typedef struct {
    int type;
    char text[256];
    char url[128];
    int x, y, w, h;
} HtmlElement;

typedef struct {
    int active;
    char url[256];
    char history[MAX_HISTORY][256];
    int hist_idx;
    int hist_count;
    HtmlElement *elements;
    int num_elements;
    int scroll_y;
    int max_scroll_y;
    int scroll_x;
    int max_scroll_x;
} BrowserTab;

BrowserTab tabs[MAX_TABS];
int active_tab = -1;

char url_buf[256] = "";
int url_focused = 0;
gui_window_t *browser_win;

int is_dragging_v = 0;
int is_dragging_h = 0;

void apply_theme(int dark) {
    if (dark) {
        c_bg = 0x001E1E1E;
        c_ui = 0x002D2D2D;
        c_ui_hover = 0x003D3D3D;
        c_fg = 0x00D4D4D4;
        c_accent = 0x000A84FF;
        c_border = 0x00111111;
    } else {
        c_bg = 0x00FFFFFF;
        c_ui = 0x00F0F0F0;
        c_ui_hover = 0x00E0E0E0;
        c_fg = 0x00000000;
        c_accent = 0x00007ACC;
        c_border = 0x00CCCCCC;
    }
}


char *fetch_url(const char *url) {
    if (strncmp(url, "demo://start", 12) == 0) {
        return strdup("<body><h1>Start Page</h1><br><a href='demo://settings'>Browser Settings</a><br><br><a href='https://example.com/'>Test HTTPS (Example.com)</a><br><br><a href='https://lite.cnn.com/'>CNN Lite (No JS)</a></body>");
    }
    if (strncmp(url, "demo://settings", 15) == 0) {
        return strdup("<body><h1>Settings</h1><br><a href='theme://dark'>Apply Dark Theme</a><br><br><a href='theme://light'>Apply Light Theme</a></body>");
    }

    int is_https = (strncmp(url, "https://", 8) == 0);
    const char *p = url + (is_https ? 8 : 7);
    char host[128] = {0}, path[256] = "/";
    
    char *slash = strchr((char*)p, '/');
    if (slash) {
        strcpy(path, slash);
        int hlen = slash - p;
        if (hlen > 127) hlen = 127;
        strncpy(host, p, hlen);
        host[hlen] = '\0'; 
    } else {
        strncpy(host, p, 127);
        host[127] = '\0';
    }

    char *buf = malloc(HTTP_BUF_SIZE);
    if (!buf) return NULL;
    memset(buf, 0, HTTP_BUF_SIZE);
    int received = 0;

    char req[1024];
    sprintf(req, 
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36\r\n"
        "Connection: close\r\n\r\n", 
        path, host);

    if (is_https) {
        mbedtls_net_context server_fd;
        mbedtls_entropy_context entropy;
        mbedtls_ctr_drbg_context ctr_drbg;
        mbedtls_ssl_context ssl;
        mbedtls_ssl_config conf;

        mbedtls_net_init(&server_fd);
        mbedtls_ssl_init(&ssl);
        mbedtls_ssl_config_init(&conf);
        mbedtls_ctr_drbg_init(&ctr_drbg);
        mbedtls_entropy_init(&entropy);

        mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)"os", 2);
        
        if (mbedtls_net_connect(&server_fd, host, "443", MBEDTLS_NET_PROTO_TCP) != 0) {
            free(buf); return NULL;
        }

        mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
        mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE);
        mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
        mbedtls_ssl_setup(&ssl, &conf);
        mbedtls_ssl_set_hostname(&ssl, host);
        mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

        if (mbedtls_ssl_handshake(&ssl) == 0) {
            mbedtls_ssl_write(&ssl, (const unsigned char*)req, strlen(req));
            int bytes;
            while ((bytes = mbedtls_ssl_read(&ssl, (unsigned char*)buf + received, HTTP_BUF_SIZE - received - 1)) > 0) {
                received += bytes;
                if (received >= HTTP_BUF_SIZE - 1) break;
            }
        }
        mbedtls_ssl_close_notify(&ssl);
        mbedtls_ssl_free(&ssl);
        mbedtls_ssl_config_free(&conf);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        mbedtls_entropy_free(&entropy);
        mbedtls_net_free(&server_fd);
    } else {
        uint32_t ip = gethostbyname(host);
        if (ip == 0 || ip == 0xFFFFFFFF) { free(buf); return NULL; }
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) { free(buf); return NULL; }
        struct sockaddr_in dest; memset(&dest, 0, sizeof(dest));
        dest.sin_family = AF_INET; dest.sin_port = htons(80); dest.sin_addr.s_addr = ip;
        if (connect(sock, (struct sockaddr*)&dest, sizeof(dest)) < 0) { close(sock); free(buf); return NULL; }
        send(sock, req, strlen(req), 0);
        int bytes;
        while ((bytes = recv(sock, buf + received, HTTP_BUF_SIZE - received - 1, 0)) > 0) {
            received += bytes; if (received >= HTTP_BUF_SIZE - 1) break;
        }
        close(sock);
    }

    if (received == 0) { free(buf); return NULL; }
    buf[received] = '\0';
    char *body = strstr(buf, "\r\n\r\n");
    if (body) { body += 4; memmove(buf, body, strlen(body) + 1); }
    return buf;
}

void extract_attr(const char* tag, const char* attr, char* out, int max_len) {
    out[0] = '\0';
    char search1[32], search2[32];
    sprintf(search1, "%s=\"", attr); sprintf(search2, "%s='", attr);
    char *p = strstr((char*)tag, search1);
    if (!p) p = strstr((char*)tag, search2);
    if (p) {
        p += strlen(search1);
        int i = 0;
        while (*p && *p != '"' && *p != '\'' && i < max_len - 1) out[i++] = *p++;
        out[i] = '\0';
    }
}

void parse_simple_html(BrowserTab *t, char *html) {
    t->num_elements = 0;
    char *p = html;
    int in_link = 0, in_heading = 0;
    char current_href[128] = {0};

    while (*p && t->num_elements < MAX_ELEMENTS) {
        if (*p == '<') {
            char tag[256] = {0}; int ti = 0;
            while (*p && *p != '>' && ti < 255) tag[ti++] = *p++;
            tag[ti] = '\0'; if (*p == '>') p++;
            char tag_lower[256]; strcpy(tag_lower, tag); to_lower(tag_lower);

            if (strncmp(tag_lower, "<style", 6) == 0) {
                char *end = strstr(p, "</style>"); if (end) p = end + 8;
            } else if (strncmp(tag_lower, "<script", 7) == 0) {
                char *end = strstr(p, "</script>"); if (end) p = end + 9;
            } else if (strncmp(tag_lower, "<br", 3) == 0 || strncmp(tag_lower, "<p", 2) == 0 || strncmp(tag_lower, "<div", 4) == 0 || strncmp(tag_lower, "<li", 3) == 0) {
                t->elements[t->num_elements++].type = ELEM_NEWLINE;
            } else if (strncmp(tag_lower, "<h1", 3) == 0 || strncmp(tag_lower, "<h2", 3) == 0) {
                t->elements[t->num_elements++].type = ELEM_NEWLINE; in_heading = 1;
            } else if (strncmp(tag_lower, "</h1", 4) == 0 || strncmp(tag_lower, "</h2", 4) == 0) {
                in_heading = 0; t->elements[t->num_elements++].type = ELEM_NEWLINE;
            } else if (strncmp(tag_lower, "<a ", 3) == 0) {
                in_link = 1; extract_attr(tag, "href", current_href, 128);
            } else if (strncmp(tag_lower, "</a", 3) == 0) {
                in_link = 0; current_href[0] = '\0';
            }
        } else {
            if (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') { p++; continue; }
            char word[256] = {0}; int wi = 0;
            if (in_link || in_heading) {
                int last_space = 0;
                while (*p && *p != '<' && wi < 255) {
                    if (*p == '\n' || *p == '\r' || *p == '\t' || *p == ' ') {
                        if (!last_space && wi > 0) { word[wi++] = ' '; last_space = 1; }
                    } else {
                        word[wi++] = *p; last_space = 0;
                    }
                    p++;
                }
                if (wi > 0 && word[wi-1] == ' ') word[wi-1] = '\0';
            } else {
                while (*p && *p != ' ' && *p != '\n' && *p != '\r' && *p != '\t' && *p != '<' && wi < 255) {
                    word[wi++] = *p++;
                }
            }
            word[wi] = '\0';

            if (wi > 0 && t->num_elements < MAX_ELEMENTS) {
                t->elements[t->num_elements].type = in_heading ? ELEM_HEADING : (in_link ? ELEM_LINK : ELEM_TEXT);
                strcpy(t->elements[t->num_elements].text, word);
                if (in_link) strcpy(t->elements[t->num_elements].url, current_href);
                t->num_elements++;
            }
        }
    }
}

void layout_page(BrowserTab *t, int width) {
    int cx = 15, cy = 75;
    int max_w = width - 15;
    t->max_scroll_x = width;
    
    for (int i = 0; i < t->num_elements; i++) {
        if (t->elements[i].type == ELEM_NEWLINE) { cx = 15; cy += 22; continue; }
        int text_w = strlen(t->elements[i].text) * 8;
        if (t->elements[i].type == ELEM_HEADING) text_w += strlen(t->elements[i].text);
        
        if (cx + text_w > max_w && cx > 15 && t->elements[i].type != ELEM_LINK && t->elements[i].type != ELEM_HEADING) {
            cx = 15;
            cy += 18;
        }
        
        t->elements[i].x = cx; t->elements[i].y = cy; t->elements[i].w = text_w;
        t->elements[i].h = (t->elements[i].type == ELEM_HEADING) ? 16 : 12;
        
        cx += text_w + 8;
        if (cx > t->max_scroll_x) t->max_scroll_x = cx;
    }
    t->max_scroll_y = cy + 40;
}

void resolve_url(const char *base_url, const char *href, char *out_url) {
    if (strncmp(href, "http", 4) == 0 || strncmp(href, "demo", 4) == 0 || strncmp(href, "theme", 5) == 0) { strcpy(out_url, href); return; }
    char temp_base[256]; strcpy(temp_base, base_url);
    char *last_slash = strrchr(temp_base, '/');
    if (href[0] == '/') {
        char *host_end = strchr(temp_base + 8, '/'); if (host_end) *host_end = '\0';
        sprintf(out_url, "%s%s", temp_base, href);
    } else {
        if (last_slash && last_slash > temp_base + 7) *last_slash = '\0';
        sprintf(out_url, "%s/%s", temp_base, href);
    }
}

void go_to(int t_idx, const char *url, int add_to_hist) {
    if (t_idx < 0 || t_idx >= MAX_TABS) return;
    BrowserTab *t = &tabs[t_idx];
    
    if (strncmp(url, "theme://", 8) == 0) {
        if (strstr((char*)url, "dark")) apply_theme(1);
        else apply_theme(0);
        return;
    }

    strcpy(t->url, url);
    if (t_idx == active_tab) strcpy(url_buf, url);
    
    gui_draw_rect(browser_win, 0, 60, browser_win->w, browser_win->h - 60, c_bg);
    gui_draw_string(browser_win, 20, 80, "Loading...", c_fg);
    gui_render(browser_win);

    char *html = fetch_url(url);
    if (html) {
        parse_simple_html(t, html);
        free(html);
    } else {
        t->num_elements = 0;
        t->elements[t->num_elements].type = ELEM_HEADING; strcpy(t->elements[t->num_elements].text, "Error"); t->num_elements++;
        t->elements[t->num_elements].type = ELEM_NEWLINE; t->num_elements++;
        t->elements[t->num_elements].type = ELEM_TEXT; strcpy(t->elements[t->num_elements].text, "Failed to fetch page. Network error or WAF block."); t->num_elements++;
    }
    
    t->scroll_y = 0;
    t->scroll_x = 0;
    layout_page(t, browser_win->w);

    if (add_to_hist && t->hist_idx < MAX_HISTORY - 1) {
        t->hist_idx++; strcpy(t->history[t->hist_idx], url); t->hist_count = t->hist_idx + 1;
    }
}

void new_tab(const char* url) {
    for (int i = 0; i < MAX_TABS; i++) {
        if (!tabs[i].active) {
            tabs[i].active = 1;
            tabs[i].hist_idx = -1;
            tabs[i].hist_count = 0;
            tabs[i].num_elements = 0;
            tabs[i].scroll_y = 0;
            tabs[i].scroll_x = 0;
            if (!tabs[i].elements) {
                tabs[i].elements = malloc(sizeof(HtmlElement) * MAX_ELEMENTS);
                if(tabs[i].elements) memset(tabs[i].elements, 0, sizeof(HtmlElement) * MAX_ELEMENTS);
            }
            active_tab = i;
            go_to(i, url, 1);
            return;
        }
    }
}

void close_tab(int t_idx) {
    if (t_idx < 0 || t_idx >= MAX_TABS || !tabs[t_idx].active) return;
    tabs[t_idx].active = 0;
    if (tabs[t_idx].elements) {
        free(tabs[t_idx].elements);
        tabs[t_idx].elements = NULL;
    }
    if (active_tab == t_idx) {
        active_tab = -1;
        for (int i = MAX_TABS - 1; i >= 0; i--) {
            if (tabs[i].active) { active_tab = i; strcpy(url_buf, tabs[i].url); break; }
        }
        if (active_tab == -1) browser_win->closed = 1;
    }
}

int gui_textfield_safari(gui_window_t *win, int x, int y, int w, int h, char *text_buffer, int max_len, int *is_focused) {
    int hovered = (win->mx >= x && win->mx <= x + w && win->my >= y && win->my <= y + h);
    if (win->clicked) *is_focused = hovered;

    if (*is_focused) {
        int len = strlen(text_buffer);
        if (win->char_input == '\b' && len > 0) text_buffer[len - 1] = '\0';
        else if (win->char_input >= 32 && win->char_input <= 126 && len < max_len - 1) {
            text_buffer[len] = win->char_input; text_buffer[len+1] = '\0';
        }
    }

    gui_draw_rounded_rect(win, x, y, w, h, 6, *is_focused ? c_accent : c_border);
    gui_draw_rounded_rect(win, x + 1, y + 1, w - 2, h - 2, 5, c_bg);
    gui_draw_string(win, x + 10, y + (h - 8) / 2, text_buffer, c_fg);
    
    if (*is_focused && (get_ticks() / 500) % 2 == 0) {
        gui_draw_rect(win, x + 10 + strlen(text_buffer)*8, y + 6, 2, h - 12, c_accent);
    }
    return *is_focused;
}

int main(int argc, char** argv) {
    apply_theme(1);
    for(int i=0; i<MAX_TABS; i++) {
        tabs[i].active = 0;
        tabs[i].elements = NULL;
    }
    
    browser_win = gui_create_window("Browser", 700, 500);
    if (!browser_win) return 1;
    gui_set_resizable(browser_win, 1);

    new_tab("demo://start");
    int last_w = browser_win->w;

    while (!browser_win->closed) {
        gui_update(browser_win);

        if (active_tab >= 0 && tabs[active_tab].elements) {
            BrowserTab *t = &tabs[active_tab];
            
            int view_h = browser_win->h - 60;
            int max_sy = t->max_scroll_y - view_h;
            if (max_sy < 0) max_sy = 0;
            
            int view_w = browser_win->w;
            int max_sx = t->max_scroll_x - view_w;
            if (max_sx < 0) max_sx = 0;

            if (browser_win->mbtn & 1) {
                if (browser_win->clicked) {
                    if (max_sy > 0 && browser_win->mx >= browser_win->w - 10 && browser_win->my >= 60 && browser_win->my <= browser_win->h - 10) is_dragging_v = 1;
                    if (max_sx > 0 && browser_win->my >= browser_win->h - 10 && browser_win->mx >= 0 && browser_win->mx <= browser_win->w - 10) is_dragging_h = 1;
                }
                if (is_dragging_v && max_sy > 0) {
                    int track_h = view_h - 10;
                    int sh = (view_h * track_h) / t->max_scroll_y;
                    if (sh < 20) sh = 20;
                    int rel_y = browser_win->my - 60 - (sh / 2);
                    if (rel_y < 0) rel_y = 0;
                    if (rel_y > track_h - sh) rel_y = track_h - sh;
                    t->scroll_y = (rel_y * max_sy) / (track_h - sh);
                }
                if (is_dragging_h && max_sx > 0) {
                    int track_w = view_w - 10;
                    int sw = (view_w * track_w) / t->max_scroll_x;
                    if (sw < 20) sw = 20;
                    int rel_x = browser_win->mx - (sw / 2);
                    if (rel_x < 0) rel_x = 0;
                    if (rel_x > track_w - sw) rel_x = track_w - sw;
                    t->scroll_x = (rel_x * max_sx) / (track_w - sw);
                }
            } else {
                is_dragging_v = 0;
                is_dragging_h = 0;
            }

            if (browser_win->scroll_z != 0) {
                t->scroll_y += browser_win->scroll_z * 40;
                if (t->scroll_y < 0) t->scroll_y = 0;
                if (t->scroll_y > max_sy) t->scroll_y = max_sy;
            }

            if (browser_win->w != last_w) { layout_page(t, browser_win->w); last_w = browser_win->w; }

            if (browser_win->clicked && browser_win->my > 60 && browser_win->my < browser_win->h - 10 && browser_win->mx < browser_win->w - 10) {
                for (int i = 0; i < t->num_elements; i++) {
                    if (t->elements[i].type == ELEM_LINK) {
                        int sx = t->elements[i].x - t->scroll_x;
                        int sy = t->elements[i].y - t->scroll_y;
                        if (browser_win->mx >= sx && browser_win->mx <= sx + t->elements[i].w &&
                            browser_win->my >= sy && browser_win->my <= sy + t->elements[i].h) {
                            char next_url[256]; resolve_url(t->url, t->elements[i].url, next_url);
                            go_to(active_tab, next_url, 1); break;
                        }
                    }
                }
            }

            if (url_focused && browser_win->key_code == KEY_ENTER) { go_to(active_tab, url_buf, 1); url_focused = 0; }
        }

        gui_draw_rect(browser_win, 0, 0, browser_win->w, browser_win->h, c_bg);
        
        gui_draw_rect(browser_win, 0, 0, browser_win->w, 30, c_ui);
        int tx = 0;
        for (int i = 0; i < MAX_TABS; i++) {
            if (!tabs[i].active) continue;
            int tw = 120;
            int thov = (browser_win->mx >= tx && browser_win->mx <= tx + tw && browser_win->my >= 0 && browser_win->my <= 30);
            gui_draw_rect(browser_win, tx, 0, tw, 30, (i == active_tab) ? c_bg : (thov ? c_ui_hover : c_ui));
            gui_draw_rect(browser_win, tx + tw - 1, 0, 1, 30, c_border);
            
            char title[16];
            strncpy(title, tabs[i].url, 12); title[12] = '\0';
            gui_draw_string(browser_win, tx + 10, 11, title, c_fg);
            
            int xhov = (browser_win->mx >= tx + tw - 20 && browser_win->mx <= tx + tw - 5 && browser_win->my >= 5 && browser_win->my <= 25);
            gui_draw_string(browser_win, tx + tw - 15, 11, "x", xhov ? c_accent : c_fg);
            
            if (browser_win->clicked && thov) {
                if (xhov) close_tab(i);
                else { active_tab = i; strcpy(url_buf, tabs[i].url); }
            }
            tx += tw;
        }
        
        int phov = (browser_win->mx >= tx && browser_win->mx <= tx + 30 && browser_win->my >= 0 && browser_win->my <= 30);
        gui_draw_rect(browser_win, tx, 0, 30, 30, phov ? c_ui_hover : c_ui);
        gui_draw_string(browser_win, tx + 11, 11, "+", c_fg);
        if (browser_win->clicked && phov) new_tab("demo://start");

        gui_draw_rect(browser_win, 0, 30, browser_win->w, 30, c_ui);
        gui_draw_rect(browser_win, 0, 60, browser_win->w, 1, c_border);

        if (active_tab >= 0 && tabs[active_tab].elements) {
            BrowserTab *t = &tabs[active_tab];
            int back_hover = (t->hist_idx > 0) && (browser_win->mx >= 10 && browser_win->mx <= 40 && browser_win->my >= 35 && browser_win->my <= 55);
            gui_draw_rounded_rect(browser_win, 10, 33, 30, 24, 4, back_hover ? c_ui_hover : c_ui);
            gui_draw_string(browser_win, 20, 41, "<", (t->hist_idx > 0) ? c_fg : c_border);
            if (back_hover && browser_win->clicked) { t->hist_idx--; go_to(active_tab, t->history[t->hist_idx], 0); }

            int fwd_hover = (t->hist_idx < t->hist_count - 1) && (browser_win->mx >= 45 && browser_win->mx <= 75 && browser_win->my >= 35 && browser_win->my <= 55);
            gui_draw_rounded_rect(browser_win, 45, 33, 30, 24, 4, fwd_hover ? c_ui_hover : c_ui);
            gui_draw_string(browser_win, 55, 41, ">", (t->hist_idx < t->hist_count - 1) ? c_fg : c_border);
            if (fwd_hover && browser_win->clicked) { t->hist_idx++; go_to(active_tab, t->history[t->hist_idx], 0); }

            int url_w = browser_win->w - 140; if (url_w < 100) url_w = 100;
            gui_textfield_safari(browser_win, 85, 33, url_w, 24, url_buf, 250, &url_focused);
            if (gui_button(browser_win, 85 + url_w + 10, 33, 35, 24, "GO", 1)) { go_to(active_tab, url_buf, 1); url_focused = 0; }

            for (int i = 0; i < t->num_elements; i++) {
                if (t->elements[i].type == ELEM_NEWLINE) continue;
                int sx = t->elements[i].x - t->scroll_x;
                int sy = t->elements[i].y - t->scroll_y;
                
                if (sy + t->elements[i].h >= 60 && sy <= browser_win->h - 10 && sx + t->elements[i].w >= 0 && sx <= browser_win->w - 10) {
                    uint32_t fg = (t->elements[i].type == ELEM_LINK) ? c_accent : c_fg;
                    if (t->elements[i].type == ELEM_LINK && browser_win->mx >= sx && browser_win->mx <= sx + t->elements[i].w && browser_win->my >= sy && browser_win->my <= sy + t->elements[i].h) {
                        gui_draw_rect(browser_win, sx, sy + 10, t->elements[i].w, 1, c_accent);
                    }
                    gui_draw_string(browser_win, sx, sy, t->elements[i].text, fg);
                    if (t->elements[i].type == ELEM_HEADING) gui_draw_string(browser_win, sx + 1, sy, t->elements[i].text, fg);
                }
            }

            int view_h = browser_win->h - 60;
            int max_sy = t->max_scroll_y - view_h;
            if (max_sy > 0) {
                int track_h = view_h - 10;
                int sh = (view_h * track_h) / t->max_scroll_y;
                if (sh < 20) sh = 20;
                int sb_y = 60 + (t->scroll_y * (track_h - sh)) / max_sy;
                gui_draw_rounded_rect(browser_win, browser_win->w - 8, sb_y, 6, sh, 3, is_dragging_v ? c_ui_hover : c_border);
            }

            int view_w = browser_win->w;
            int max_sx = t->max_scroll_x - view_w;
            if (max_sx > 0) {
                int track_w = view_w - 10;
                int sw = (view_w * track_w) / t->max_scroll_x;
                if (sw < 20) sw = 20;
                int sb_x = (t->scroll_x * (track_w - sw)) / max_sx;
                gui_draw_rounded_rect(browser_win, sb_x, browser_win->h - 8, sw, 6, 3, is_dragging_h ? c_ui_hover : c_border);
            }
        }

        gui_render(browser_win);
        yield();
    }

    for (int i = 0; i < MAX_TABS; i++) {
        if (tabs[i].elements) free(tabs[i].elements);
    }

    gui_destroy_window(browser_win);
    return 0;
}
