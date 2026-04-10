// rtc.h — ARM generic timer utilities (CNTPCT-based ticks, delays, periodic callbacks)

#ifndef LEG_RTC_H
#define LEG_RTC_H

#include "types.h"



u64 rtc_ticks(void);

void delay_us(u32 usec);

// periodic tick — fn is called every interval_us from IRQ context
void timer_set_tick(u32 interval_us, void (*fn)(void));

void timer_clear_tick(void);

// called from IRQ handler when timer fires
void rtc_timer_fired(void);

#endif // LEG_RTC_H
