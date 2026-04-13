#include "libc/stdlib.h"

#include "libc/math.h"

[[gnu::const]]
static int get_symbol(const u8 digit) {
    return digit < 0xA ? '0' + digit : 'a' + digit - 0xA;
}

char *utoa(u32 value, char *buffer, const u8 radix) {
    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return buffer;
    }

    const auto digit_count = log(value, radix);
    auto       i = 0u;
    do {
        const u8 digit = value % radix;
        buffer[digit_count - i++] = get_symbol(digit);
    } while (value /= radix);

    buffer[i] = '\0';
    return buffer;
}

char *itoa(i32 value, char *buffer, const u8 radix) {
    if (value < 0) {
        *buffer++ = '-';
        value = -value;
    }
    utoa(value, buffer, radix);
    return buffer;
}

[[gnu::const]]
u16 hex8(const u8 digit) {
    const int div = digit / 16, mod = digit % 16;
    return get_symbol(div) | get_symbol(mod) << 8;
}

char *hex32le(const u32 value, char str[9]) {
    const auto alias = (u16 *) str;
    alias[0] = hex8(value >> 0u & 0xff);
    alias[1] = hex8(value >> 8u & 0xff);
    alias[2] = hex8(value >> 16 & 0xff);
    alias[3] = hex8(value >> 24 & 0xff);
    str[8] = '\0';
    return str;
}

char *hex32be(const u32 value, char str[9]) {
    const auto alias = (u16 *) str;
    alias[3] = hex8(value >> 0u & 0xff);
    alias[2] = hex8(value >> 8u & 0xff);
    alias[1] = hex8(value >> 16 & 0xff);
    alias[0] = hex8(value >> 24 & 0xff);
    str[8] = '\0';
    return str;
}

[[gnu::pure]]
double atof(const char *s) {
    double result = 0.0, frac = 1.0;
    bool   neg = false;
    if (*s == '-') {
        neg = true;
        s++;
    } else if (*s == '+')
        s++;
    while (*s >= '0' && *s <= '9')
        result = result * 10.0 + (*s++ - '0');
    if (*s == '.') {
        s++;
        while (*s >= '0' && *s <= '9') {
            frac /= 10.0;
            result += (*s++ - '0') * frac;
        }
    }
    return neg ? -result : result;
}
