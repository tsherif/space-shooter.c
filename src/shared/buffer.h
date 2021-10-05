#ifndef _BUFFER_H_
#define _BUFFER_H_
#include <stdint.h>

typedef struct {
    uint8_t* data;
    uint32_t size;
} BufferBuffer;

void buffer_free(BufferBuffer* buffer);

#endif