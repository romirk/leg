//
// Created by Romir Kulshrestha on 03/06/2025.
//

#ifndef STRING_H
#define STRING_H
#include <stddef.h>

#define TOKEN_CONCAT_INT(x, y) x##y
#define TOKEN_CONCAT(x, y) TOKEN_CONCAT_INT(x, y)
#define UNIQUE(tok) TOKEN_CONCAT(tok, __COUNTER__)

#define STR_SWITCH_BEGIN(str) do { const auto _switch_str = (str); if (false) __builtin_unreachable();
#define STR_SWITCH_CASE(val) else if(!strcmp(_switch_str, val))
#define STR_SWITCH_STARTS_WITH(val, n) else if (!strncmp(_switch_str, val, n))
#define STR_SWITCH_DEFAULT(val) else
#define STR_SWITCH_END } while(false)

int strlen(const char *s);

int strcmp(const char *s1, const char *s2);

int strncmp(const char *s1, const char *s2, size_t n);

char *strcpy(char *dest, const char *src);

#endif //STRING_H
