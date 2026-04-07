//
// Created by Romir Kulshrestha on 17/08/2025.
//

#ifndef LEG_RTC_H
#define LEG_RTC_H

#include "types.h"

#define RTC_BASE    0x09010000

u64 rtc_ticks(void);
void delay_us(u32 usec);

// called from IRQ handler when timer fires
void rtc_timer_fired(void);

#endif //LEG_RTC_H