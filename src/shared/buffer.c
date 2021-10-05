#include <malloc.h>
#include "buffer.h"

void buffer_free(BufferBuffer* buffer) {
	if (!buffer->data) {
		return;
	}

	free(buffer->data);
	buffer->data = NULL;
	buffer->size = 0;
}