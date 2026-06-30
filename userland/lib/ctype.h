/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/lib/ctype.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#ifndef _FAKE_CTYPE_H
#define _FAKE_CTYPE_H
static inline int isspace(int c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
static inline int isdigit(int c) { return c >= '0' && c <= '9'; }
static inline int isalpha(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static inline int isalnum(int c) { return isalpha(c) || isdigit(c); }
#endif
