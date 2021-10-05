#include <malloc.h>
#include "data.h"

void data_freeBuffer(DataBuffer* buffer) {
	if (!buffer->data) {
		return;
	}

	free(buffer->data);
	buffer->data = NULL;
	buffer->size = 0;
}

void data_freeImage(DataImage* image) {
	data_freeBuffer(&image->buffer);

	image->width = 0;
	image->height = 0;
}