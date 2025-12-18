#ifndef SPEAKER_H
#define SPEAKER_H

#include "interrupt/idt/idt.h"

void beep(unsigned int frequency);
void stop_beep(void);
void beep_timed(unsigned int frequency, unsigned int duration_ms);

#endif