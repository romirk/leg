#ifndef TYPES_H
#define TYPES_H

typedef long long unsigned int u64;
typedef unsigned int           u32, uptr, size_t;
typedef unsigned short         u16;
typedef unsigned char          u8, byte;
typedef long long int          i64;
typedef int                    i32, iptr;
typedef short                  i16;
typedef signed char            i8;

#define U32_MAX ((u32) ~0)
#define U64_MAX ((u64) ~0)

#define loop for (;;)

#endif // TYPES_H
