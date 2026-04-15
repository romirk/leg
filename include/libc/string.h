//
// Created by Romir Kulshrestha on 14/04/2026.
//

#ifndef LEG_STRING_H
#define LEG_STRING_H

#include "types.h"

typedef struct {
    u32   len;
    char *str;
} str_t;

str_t *str_new(const char *);
str_t *str_dup(const str_t *);
void   str_free(str_t *);
char  *into_char(const str_t *);

u32 str_cpy(str_t *dest, const str_t *src);
u32 str_cat(str_t *dest, const str_t *src);
u32 str_cmp(const str_t *, const str_t *);

u32 str_trim(str_t *);

#endif // LEG_STRING_H
