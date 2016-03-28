#pragma once

#include <inttypes.h>

typedef int8_t   int8;
typedef int16_t  int16;
typedef int32_t  int32;
typedef int64_t  int64;
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif