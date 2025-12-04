#ifndef SYSTEM_H
#define SYSTEM_H

#include "drivers/vga/vga.h"
#include "drivers/timer/timer.h"

void cmd_colortest(void);
void cmd_banner(void);
void cmd_sysinfo(void);
void cmd_echo(char **tokens);
void cmd_calc(char **tokens);
void cmd_time(void);
void cmd_ascii(void);
void cmd_box(void);
void cmd_settextcolor(char **tokens);
void cmd_help(int page);
void cmd_setbgcolor(char** tokens);

#endif