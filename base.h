#pragma once

#ifndef CONFIG_BIG_ENDIAN
#define CONFIG_LITTLE_ENDIAN
#endif

typedef unsigned char       U8;
typedef signed   char       S8;
typedef unsigned short      U16;
typedef signed   short      S16;

typedef unsigned int        U32;
typedef signed   int        S32;
typedef unsigned long long  U64;
typedef signed   long long  S64;

typedef unsigned int        SZ;

typedef union
{
    U16 u16;
    S16 s16;
    U8 bytes[2];
    struct { U8 b1, b0; } u8;
    struct { S8 b1; U8 b0; } s8;
} M16;
typedef union
{
    U32 u32;
    S32 s32;
    U8 bytes[4];
    struct { U16 w1, w0; } u16;
    struct { S16 w1; U16 w0; } s16;
    struct { U8 b3, b2, b1, b0; } u8;
    struct { M16 hi, lo; } m16;
} M32;
typedef union
{
    U64 u64;
    S64 s64;
    U8 bytes[8];
    struct { U32 hi, lo; } u32;
    struct { S32 hi; U32 lo; } s32;
    struct { U16 w3, w2, w1, w0; } u16;
    struct { U8 b7, b6, b5, b4, b3, b2, b1, b0; } u8;
    struct { M32 hi, lo; } m32;
} M64;
#define IN
#define OUT
