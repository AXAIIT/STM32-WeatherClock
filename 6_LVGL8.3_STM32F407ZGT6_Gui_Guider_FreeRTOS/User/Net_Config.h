#ifndef __NET_CONFIG_H
#define __NET_CONFIG_H

#include <stdint.h>

typedef uint8_t U8;
typedef uint32_t U32;

typedef struct {
    U32 hash;
    const U8 *data;
} HTTP_FILE;

#endif
