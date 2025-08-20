//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "main.h"

#include "linker.h"
#include "logs.h"
#include "memory.h"
#include "rtc.h"
#include "stdio.h"
#include "utils.h"

[[noreturn]]
[[gnu::used]]
void kmain() {
    info("RTC enabled: %t | RTC masked: %t", *(bool *) (RTC_BASE + 0x00c), *(bool *) (RTC_BASE + 0x010));
    info("RTC ID: %x", *(u32 *) (RTC_BASE + 0xfe0));
    // u32 clk = *(volatile u32 *) RTC_BASE;
    // for (int i = 0; i < 10; ++i) {
    //     u32 nxt = clk;
    //     while (nxt == clk) {
    //         nxt = *(volatile u32 *) RTC_BASE;
    //     }
    //     clk = nxt;
    //     dbg("RTC clk: %p", clk);
    // }

    warn("HALT");
    limbo;
}
