/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/app.ld
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

ENTRY(_start)
OUTPUT_FORMAT(binary)

SECTIONS
{
    . = 0x60000000;

    .text : {
        *(.text.entry) 
        *(.text)
    }

    .data : {
        *(.data)
    }

    .rodata : {
        *(.rodata)
    }

    .bss : {
        *(.bss)
    }
}