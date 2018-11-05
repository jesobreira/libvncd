/* RFBPixelFormat.cpp */

/*
 * Copyright (c) 2015, the libvncd author
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "RFBPixelFormat.hpp"
#include <memory>
#include "asio_wrapper.h"

#define LITTLE_ENDIAN 0x41424344UL 
#define BIG_ENDIAN    0x44434241UL
#define PDP_ENDIAN    0x42414443UL
#define ENDIAN_ORDER  ('ABCD') 

std::string RFBPixelFormat::renderStruct() {

	char buff[16] = { 0 };
	memcpy(buff, this, 16);

#if ENDIAN_ORDER == LITTLE_ENDIAN
	// The message must always be in big endian format, swap the uint16_t's around
	uint8_t tmp;
		
	tmp = buff[4]; buff[4] = buff[5]; buff[5] = tmp;
	tmp = buff[6]; buff[6] = buff[7]; buff[7] = tmp;
	tmp = buff[8]; buff[8] = buff[9]; buff[9] = tmp;
#endif
	
	return std::string(buff, sizeof(buff));

}

void RFBPixelFormat::setFrom(std::string buff) {
	if (buff.length() != 16) {
		return; // Unexpected
	}

#if ENDIAN_ORDER == LITTLE_ENDIAN
	// The message must always be in big endian format, swap the uint16_t's around
	uint8_t tmp;

	tmp = buff[4]; buff[4] = buff[5]; buff[5] = tmp;
	tmp = buff[6]; buff[6] = buff[7]; buff[7] = tmp;
	tmp = buff[8]; buff[8] = buff[9]; buff[9] = tmp;
#endif

	memcpy(this, buff.c_str(), 16);
}

uint32_t swapbytes32(uint32_t input) {
	uint8_t a = (input >> 0) & 0xFF;
	uint8_t b = (input >> 8) & 0xFF;
	uint8_t c = (input >> 16) & 0xFF;
	uint8_t d = (input >> 24) & 0xFF;

	uint32_t ret = (a << 24) | (b << 16) | (c << 8) | d;
	return ret;
}

uint16_t swapbytes16(uint16_t input) {
	uint8_t a = (input >> 0) & 0xFF;
	uint8_t b = (input >> 8) & 0xFF;

	uint16_t ret = (a << 8) | b;
	return ret;
}

void RFBPixelFormat::writeCommon(char** ptr, uint8_t r, uint8_t g, uint8_t b) {

	// scale to max
	uint8_t rscaled = (r * redMax) / 255;
	uint8_t gscaled = (g * greenMax) / 255;
	uint8_t bscaled = (b * blueMax) / 255;

	if (bitsPerPixel == 32){
		uint32_t write = 0;

		write |= rscaled << redShift;
		write |= gscaled << greenShift;
		write |= bscaled << blueShift;

		if (
			(bigEndianFlag && ENDIAN_ORDER == LITTLE_ENDIAN) ||
			(!bigEndianFlag && ENDIAN_ORDER == BIG_ENDIAN)
			) {
			write = swapbytes32(write); // can't just use ntohl
		}

		memcpy(*ptr, &write, sizeof(uint32_t));

	} else if (bitsPerPixel == 16) {
		uint16_t write = 0;

		write |= rscaled << redShift;
		write |= gscaled << greenShift;
		write |= bscaled << blueShift;

		if (
			(bigEndianFlag && ENDIAN_ORDER == LITTLE_ENDIAN) ||
			(!bigEndianFlag && ENDIAN_ORDER == BIG_ENDIAN)
			) {
			write = swapbytes16(write); // can't just use ntohl
		}

		memcpy(*ptr, &write, sizeof(uint16_t));

	} else if (bitsPerPixel == 8) {
		uint8_t write = 0;

		write |= rscaled << redShift;
		write |= gscaled << greenShift;
		write |= bscaled << blueShift;

		// No endianness for a single byte

		**ptr = write;

	} else {
		// invalid
		return;

	}

	// Advance ptr

}

void RFBPixelFormat::writeTo(char** ptr, uint8_t r, uint8_t g, uint8_t b) {
	writeCommon(ptr, r, g, b);
	*ptr += (bitsPerPixel / 8);
}

void RFBPixelFormat::writeCpixelTo(char** ptr, uint8_t r, uint8_t g, uint8_t b) {
	writeCommon(ptr, r, g, b);
	*ptr += (bitDepth / 8);
}

void RFBPixelFormat::copyRect(uint8_t* fromRGBX32, uint16_t sourceImageWidth, char* dest, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {

	char* nextPos = dest;

	for (size_t ypos = y; ypos < (size_t)y + (size_t)h; ++ypos) {
		for (size_t xpos = x; xpos < (size_t)x + (size_t)w; ++xpos) {

			writeTo(
				&nextPos,
				fromRGBX32[(xpos + (ypos * sourceImageWidth)) * 4 + 0],
				fromRGBX32[(xpos + (ypos * sourceImageWidth)) * 4 + 1],
				fromRGBX32[(xpos + (ypos * sourceImageWidth)) * 4 + 2]
			);

		}
	}

}

void RFBPixelFormat::copyRectCpixel(uint8_t* fromRGBX32, uint16_t sourceImageWidth, char* dest, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {

	char* nextPos = dest;

	for (size_t ypos = y; ypos < (size_t)y + (size_t)h; ++ypos) {
		for (size_t xpos = x; xpos < (size_t)x + (size_t)w; ++xpos) {

			writeCpixelTo(
				&nextPos,
				fromRGBX32[(xpos + (ypos * sourceImageWidth)) * 4 + 0],
				fromRGBX32[(xpos + (ypos * sourceImageWidth)) * 4 + 1],
				fromRGBX32[(xpos + (ypos * sourceImageWidth)) * 4 + 2]
			);

		}
	}

}