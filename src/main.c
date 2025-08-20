//
// Created by Romir Kulshrestha on 20/08/2025.
//

#include "main.h"

#include <logs.h>
#include <rtc.h>

int main() {
    info("RTC enabled: %t | RTC masked: %t", *(bool *) (RTC_BASE + 0x00c), *(bool *) (RTC_BASE + 0x010));
    info("RTC ID: %x", *(u32 *) (RTC_BASE + 0xfe0));
    return 1;
}
