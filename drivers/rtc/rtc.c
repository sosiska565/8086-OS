/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/drivers/rtc/rtc.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include "drivers/rtc/rtc.h"
#include "drivers/io/io.h"

uint8_t rtc_read(uint8_t reg){
    outb(RTC_ADDRES, reg);
    return inb(RTC_DATA);
}

static uint8_t bcd_to_bin(uint8_t bcd){
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

void rtc_init(void){
    outb(RTC_ADDRES, RTC_STATUS_B);
    uint8_t prev = inb(RTC_DATA);
    outb(RTC_ADDRES, RTC_STATUS_B);
    outb(RTC_DATA, prev | 0x02);
}

struct time rtc_get_time(void){
    struct time t;

    while (rtc_read(RTC_STATUS_A) & 0x80);

    t.second = bcd_to_bin(rtc_read(RTC_SECOND));
    t.minute = bcd_to_bin(rtc_read(RTC_MINUTE));
    t.hour = bcd_to_bin(rtc_read(RTC_HOUR));
    t.day = bcd_to_bin(rtc_read(RTC_DAY));
    t.month = bcd_to_bin(rtc_read(RTC_MONTH));
    t.year = bcd_to_bin(rtc_read(RTC_YEAR));

    return t;
}
