#ifndef UTILS_H
#define UTILS_H

typedef struct {
    char* key;
    char* value; 
} ConfigEntry;

typedef struct {
    ConfigEntry* entries;
    int count;
    int capacity;
} Config;

void srand(unsigned long seed);
unsigned long random(void);
void get_cpu_vendor(char *buffer);
void panic(char *err);
int is_space(char c);
char* trim_whitespace(char* str);

Config* config_parse(char* buffer);
char* config_get_value(Config* cfg, const char* key);
void config_free(Config* cfg);
void config_save(char *filename, Config *cfg);
void config_set_value(Config *cfg, char *key, char *new_val);
int get_pixels_in_string(char *str);

#endif