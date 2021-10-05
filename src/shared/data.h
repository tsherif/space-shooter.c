#ifndef _DATA_H_
#define _DATA_H_
#include <stdint.h>
#include "macros.h"

typedef struct {
    uint8_t* data;
    uint32_t size;
} DataBuffer;

typedef struct {
    MIXIN_STRUCT(DataBuffer, buffer);
    int32_t width;
    int32_t height;
} DataImage;

void data_freeBuffer(DataBuffer* buffer);
void data_freeImage(DataImage* image);

#endif