#include "utils.h"
#include <malloc.h>
#include "../shared/platform-interface.h"

// NOTE(Tarek): Hardcoded to load 32bpp BGRA  
bool utils_bmpToImage(uint8_t* bmpData, UtilsImage* image) {
	uint16_t type = *(uint16_t *) bmpData;

	if (type != 0x4d42) {
		platform_debugLog("utils_bmpToRgba: Invalid BMP data.");
		return false;
	}
	uint32_t imageOffset   = *(uint32_t *) (bmpData + 10);
	
	uint32_t dibHeaderSize = *(uint32_t *) (bmpData + 14);
	if (dibHeaderSize < 70) {
		platform_debugLog("utils_bmpToRgba: Unsupported DIB header.");
		return false;
	}

	int32_t width          = *(int32_t *)  (bmpData + 18);
	int32_t height         = *(int32_t *)  (bmpData + 22);

	uint16_t bpp           = *(uint16_t *) (bmpData + 28);
	if (bpp != 32) {
		platform_debugLog("utils_bmpToRgba: Unsupported bpp, must be 32.");
		return false;
	}

	uint32_t compression   = *(uint32_t *) (bmpData + 30);
	if (compression != 3) {
		platform_debugLog("utils_bmpToRgba: Unsupported compression, must be BI_BITFIELDS (3).");
		return false;
	}

	uint32_t redMask       = *(uint32_t *) (bmpData + 54);
	uint32_t greenMask     = *(uint32_t *) (bmpData + 58);
	uint32_t blueMask      = *(uint32_t *) (bmpData + 62);
	uint32_t alphaMask     = *(uint32_t *) (bmpData + 66);

	if (redMask != 0x00ff0000 || greenMask != 0x0000ff00 || blueMask != 0x000000ff || alphaMask != 0xff000000) {
		platform_debugLog("utils_bmpToRgba: Unsupported pixel layout, must be BGRA.");
		return false;
	}


	uint8_t* bmpImage = bmpData + imageOffset;
	
	uint32_t numPixels = width * height;
	uint8_t* imageData = (uint8_t *) malloc(numPixels * 4);

	for (uint32_t i = 0; i < numPixels;  ++i) {
		uint16_t row = i / width;
		uint16_t col = i % width;
		uint16_t mirrorRow = height - row - 1;
		uint32_t mirrorI = mirrorRow * width + col;

		uint32_t byteI = i * 4;
	    uint8_t b = bmpImage[byteI];
	    uint8_t g = bmpImage[byteI + 1];
	    uint8_t r = bmpImage[byteI + 2];
	    uint8_t a = bmpImage[byteI + 3];

		uint32_t mirrorByteI = mirrorI * 4;
	    imageData[mirrorByteI]     = r;
	    imageData[mirrorByteI + 1] = g;
	    imageData[mirrorByteI + 2] = b;
	    imageData[mirrorByteI + 3] = a;
	}

	image->data = imageData;
	image->width = (uint16_t) width;
	image->height = (uint16_t) height;

	return true;
}

void utils_freeImage(UtilsImage* image) {
	if (!image->data) {
		return;
	}

	free(image->data);
	image->data = NULL;
	image->width = 0;
	image->height = 0;
}