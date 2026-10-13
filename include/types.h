#pragma once

typedef long long unsigned int u64;
typedef unsigned int           u32;
typedef unsigned short         u16;
typedef unsigned char          u8, byte;
typedef long long int          i64;
typedef int                    i32;
typedef short                  i16;
typedef signed char            i8;

#ifdef __aarch64__
typedef u64 uptr, size_t;
typedef i64 iptr;
#else
typedef u32 uptr, size_t;
typedef i32 iptr;
#endif

#define U32_MAX ((u32) ~0)
#define U64_MAX ((u64) ~0)

#define loop for (;;)
