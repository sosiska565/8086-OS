#ifndef SYSTEM_H
#define SYSTEM_H


#include <stddef.h>
#include "console.h"

extern command_t commands[];

void cmd_colortest(char **tokens);
void cmd_echo(char **tokens);
void cmd_calc(char **tokens);
void cmd_time(char **tokens);
void cmd_ascii(char **tokens);
void cmd_settextcolor(char **tokens);
void cmd_help(char **tokens);
void cmd_setbgcolor(char** tokens);
void cmd_clear(char **tokens);
void cmd_exit(char **tokens);
void cmd_memview(char **tokens);
void cmd_kmalloc(char **tokens);
void cmd_kfree(char **tokens);
void cmd_heapdump(char **tokens);
void cmd_disk_viewer(char **tokens);
void cmd_ls(char **tokens);
void cmd_cat(char **tokens);
void cmd_exec(char **tokens);
void cmd_mkfile(char **tokens);
void cmd_rm(char **tokens);
void cmd_readsystemcfg(char **tokens);
void cmd_tasklist(char **tokens);
void cmd_kill(char **tokens);
void cmd_writemode(char **tokens);
void cmd_disks(char **tokens);
void cmd_use(char **tokens);
void cmd_cd(char **tokens);
void cmd_mkdir(char **tokens);

#endif