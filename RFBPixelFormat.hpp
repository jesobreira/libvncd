/* RFBPixelFormat.hpp */

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

#pragma once
#include <string>
#include <cstdint>

struct RFBPixelFormat {

	uint8_t bitsPerPixel	= 32;
	uint8_t bitDepth		= 24; // number of useful bits in bitsPerPixel
	uint8_t bigEndianFlag	= 1;  // x86 must call ntohs/htons
	uint8_t trueColourFlag	= 1;
	uint16_t redMax			= 255;
	uint16_t greenMax		= 255;
	uint16_t blueMax		= 255;
	uint8_t redShift		= 16;
	uint8_t greenShift		= 8;
	uint8_t blueShift		= 0;
	uint8_t _unused_padding[3];

	std::string renderStruct(); // always in big-endian order, x86 must call ntohs/htons on the 16-bit fields

	void writeTo(char** ptr, uint8_t r, uint8_t g, uint8_t b);

	void writeCpixelTo(char** ptr, uint8_t r, uint8_t g, uint8_t b);

	void copyRect(uint8_t* fromRGBX32, uint16_t sourceImageWidth, char* dest, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

	void copyRectCpixel(uint8_t* fromRGBX32, uint16_t sourceImageWidth, char* dest, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

	void setFrom(std::string PIXEL_FORMAT_STRING);

protected:

	void writeCommon(char** ptr, uint8_t r, uint8_t g, uint8_t b);

};
