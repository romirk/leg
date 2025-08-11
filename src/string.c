//
// Created by Romir Kulshrestha on 03/06/2025.
//

#include "string.h"

int strlen(const char *s) {
    int len = 0;
    while (*s++) len++;
    return len;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s1 == *s2)
        s1++, s2++;
    return *s1 - *s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && *s1 == *s2)
        s1++, s2++, n--;
    if (n)
        return *s1 - *s2;
    return 0;
}

char *strcpy(char *dest, const char *src) {
    while ((*dest++ = *src++));
    return dest;
}