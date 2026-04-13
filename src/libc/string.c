#include "libc/string.h"

[[gnu::pure]]
int strlen(const char *s) {
    int len = 0;
    while (*s++)
        len++;
    return len;
}

[[gnu::pure]]
int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s1 == *s2)
        s1++, s2++;
    return *s1 - *s2;
}

[[gnu::pure]]
int strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && *s1 == *s2)
        s1++, s2++, n--;
    if (n) return *s1 - *s2;
    return 0;
}

char *strcpy(char *dest, const char *src) {
    while ((*dest++ = *src++))
        ;
    return dest;
}

int str_split(char *line, char **argv, int max_argc) {
    int argc = 0;
    while (*line && argc < max_argc) {
        while (*line == ' ' || *line == '\t')
            line++;
        if (!*line) break;
        argv[argc++] = line;
        while (*line && *line != ' ' && *line != '\t')
            line++;
        if (*line) *line++ = '\0';
    }
    return argc;
}

char *trim(char *s) {
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')
        s++;
    if (!*s) return s;
    char *end = s;
    while (*(end + 1))
        end++;
    while (end > s && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r'))
        end--;
    *(end + 1) = '\0';
    return s;
}
