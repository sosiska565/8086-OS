/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/drivers/speaker/speaker.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#ifndef SPEAKER_H
#define SPEAKER_H

void beep(unsigned int frequency);
void stop_beep(void);
void beep_timed(unsigned int frequency, unsigned int duration_ms);

#endif
