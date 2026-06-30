/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/drivers/rtc/rtc.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#ifndef RTC_H
#define RTC_H

#include<stdint.h>

struct time{
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
};

#define RTC_ADDRES 0x70
#define RTC_DATA 0x71

#define RTC_SECOND 0x00
#define RTC_MINUTE 0x02
#define RTC_HOUR 0x04
#define RTC_DAY 0x07
#define RTC_MONTH 0x08
#define RTC_YEAR 0x09
#define RTC_STATUS_A 0x0A
#define RTC_STATUS_B 0x0B

void rtc_init(void);
uint8_t rtc_read(uint8_t reg);
struct time rtc_get_time(void);

#endif
