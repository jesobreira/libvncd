/* VncdConnection.cpp */

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

#include "VncdConnection.hpp"
#include <sstream>

#include "miniz_wrapper.h"
#include "des/d3des.h"

// {{{ 

#define VNCD_MAX_RECT_SIZE_HEIGHT	512
#define VNCD_MAX_RECT_SIZE_WIDTH	512
#define VNCD_ZLIB_COMPRESSION		MZ_DEFAULT_COMPRESSION

// }}}

VncdConnection::VncdConnection(asio::ip::tcp::socket tcpConnection, VncdTimer timer) :
	tcpConnection(std::move(tcpConnection)),
	timer(std::move(timer)),
	sb_mutable(sb.prepare(4096)),
	useEncodingMode(VEM_RAW),
	zrleStream({ 0 }),
	zlibStream({ 0 })
{
	// One ZRLE stream is used for the entire protocol session
	mz_deflateInit(&zrleStream, VNCD_ZLIB_COMPRESSION);

	// One ZLIB stream is used for the entire protocol session
	mz_deflateInit(&zlibStream, VNCD_ZLIB_COMPRESSION);

}

VncdConnection::~VncdConnection() {
	
	mz_deflateEnd(&zrleStream);
	mz_deflateEnd(&zlibStream);

}

std::string VncdConnection::getBuffer(size_t numBytes) {
	const char* ptr = asio::buffer_cast<const char*>(sb.data());
	return std::string(ptr, numBytes);	
}

void VncdConnection::notifyClient_connectionAccepted() {

	setCurrentStatusMessage("Negotiating protocol version...");
	currentState = VCS_HANDSHAKE_1_SENDING_PROTOCOL;

	// Send handshake message

	tcpConnection.async_send(
		asio::buffer("RFB 003.008\n", 12),
		[this](std::error_code ec, std::size_t nb) {
			// Wait for "RFB XXX.YYY\n" response from client
			currentState = VCS_HANDSHAKE_2_WAITING_FOR_PROTOCOL_RESPONSE;			
		}
	);

	awaitProtocolMessage();
}

void VncdConnection::awaitProtocolMessage() {
	tcpConnection.async_read_some(
		sb_mutable,
		[this](std::error_code ec, std::size_t nb) {

			if (ec) {
				setCurrentStatusMessage("Network failure.");
				tcpConnection.close();
				return;

			} else {
				std::string message = getBuffer(nb);
				handleProtocolMessage(message);
				if (tcpConnection.is_open()) {
					awaitProtocolMessage(); // loop (tail call)
				}

			}

		}
	);
}

void VncdConnection::handleProtocolMessage(std::string message) {

	switch (currentState) {

		case VCS_HANDSHAKE_2_WAITING_FOR_PROTOCOL_RESPONSE: {
			if (message != "RFB 003.008\n") {
				setCurrentStatusMessage("Failed to agree on protocol version.");
				tcpConnection.close();
				return;
			}

			setCurrentStatusMessage("Negotiating protocol security...");
			currentState = VCS_HANDSHAKE_3_SENDING_SECURITY_TYPES;

			// Send (lack of) security types

			std::string securityMessage;

			if (requirePassword().size()) {
				securityMessage = "\x01\x02"; // VNC authentication
			} else {
				securityMessage = "\x01\x01"; // No authentication necessary
			}

			tcpConnection.async_send(
				asio::buffer(securityMessage.c_str(), securityMessage.size()),
				[this](std::error_code ec, std::size_t nb) {
					currentState = VCS_HANDSHAKE_4_WAITING_FOR_SECURITY_SELECTION;
				}
			);
		} break;

		case VCS_HANDSHAKE_4_WAITING_FOR_SECURITY_SELECTION: {

			if (!requirePassword().size()) {

				if (message != "\x01") {
					setCurrentStatusMessage("Failed to agree on protocol security.");
					tcpConnection.close();
					return;
				}

				setCurrentStatusMessage("Negotiating session parameters...");
				currentState = VCS_HANDSHAKE_7_SENDING_SECURITY_OK;

				// Send successful security result

				tcpConnection.async_send(
					asio::buffer("\x00\x00\x00\x00", 4),
					[this](std::error_code ec, std::size_t nb) {
						currentState = VCS_HANDSHAKE_8_WAITING_FOR_CLIENTINIT;
					}
				);

			} else {

				if (message != "\x02") {
					setCurrentStatusMessage("Failed to agree on protocol security.");
					tcpConnection.close();
					return;
				}

				setCurrentStatusMessage("Exchanging password challenge...");
				currentState = VCS_HANDSHAKE_5_SENDING_SECURITY_CHALLENGE;

				desChallengeNonce.resize(16);

				srand(time(NULL));
				for (int i = 0; i < 16; ++i) {
					desChallengeNonce[i] = rand() % 0xFF;
				}

				tcpConnection.async_send(
					asio::buffer(desChallengeNonce.c_str(), desChallengeNonce.size()),
						[this](std::error_code ec, std::size_t nb) {
						currentState = VCS_HANDSHAKE_6_WAITING_FOR_SECURITY_RESPONSE;
					}
				);
			}

		} break;

		case VCS_HANDSHAKE_6_WAITING_FOR_SECURITY_RESPONSE: {

			if (message.size() != 16) {
				setCurrentStatusMessage("Malformed authentication.");
			}
			
			std::string ourPassword = requirePassword();
			ourPassword.resize(7, '\x00'); // 7*8 = 56 bit key

			uint32_t desKeyRegister[32] = { 0 };

			d3des_cook_key((unsigned char*)ourPassword.c_str(), EN0, desKeyRegister);
			d3des_transform(desKeyRegister, (unsigned char*)desChallengeNonce.c_str() + 0, (unsigned char*)desChallengeNonce.c_str() + 0);
			d3des_transform(desKeyRegister, (unsigned char*)desChallengeNonce.c_str() + 8, (unsigned char*)desChallengeNonce.c_str() + 8);

			if (desChallengeNonce == message) {
				// Password match
				tcpConnection.async_send(
					asio::buffer("\x00\x00\x00\x00", 4), 
					[this](std::error_code ec, std::size_t nb) {
						currentState = VCS_HANDSHAKE_8_WAITING_FOR_CLIENTINIT;
					}
				);
				
			} else {
				// Password mismatch
				tcpConnection.async_send(
					asio::buffer("\x00\x00\x00\x01" "\x00\x00\x00\x0C" "Bad password", 4+4+0x0C),
					[this](std::error_code ec, std::size_t nb) {
						currentState = VCS_INVALID;
						tcpConnection.close();
					}
				);
				
			}

		} break;

		case VCS_HANDSHAKE_8_WAITING_FOR_CLIENTINIT: {
			// Validate expected packet length, but ignore content

			if (message.length() != 1) {
				setCurrentStatusMessage("Failed to agree on session parameters.");
				tcpConnection.close();
				return;
			}

			// Build ServerInit message

			char serverInit[24] = { 0 };

			uint16_t fbWidth = htons(getFrameWidth()), fbHeight = htons(getFrameHeight());
			memcpy(serverInit + 0, &fbWidth, 2);
			memcpy(serverInit + 2, &fbHeight, 2);

			std::string pxFormat = networkPixelFormat.renderStruct();
			memcpy(serverInit + 4, pxFormat.data(), 16);

			std::string desc = getSessionTitle();
			uint32_t desc_len = htonl(desc.length());
			memcpy(serverInit + 20, &desc_len, 4);

			std::string messageToSend(serverInit, sizeof(serverInit));
			messageToSend.append(desc);

			// Send ServerInit message
			currentState = VCS_HANDSHAKE_9_SENDING_SERVERINIT;

			tcpConnection.async_send(
				asio::buffer(messageToSend.c_str(), messageToSend.length()),
				[this](std::error_code ec, std::size_t nb) {
					currentState = VCS_READY;
					setCurrentStatusMessage("Connected.");
					connectionStarted();
				}
			);

		} break;

		case VCS_READY: {

			if (message.length() == 20 && message[0] == '\x00') {
				setCurrentStatusMessage("Client requested new pixel bit depth");
				networkPixelFormat.setFrom(message.substr(4));

				notifyClient_regionUpdated(0, 0, getFrameWidth(), getFrameHeight()); // redraw all

			} else if (message.length() == 10 && message[0] == '\x03') {
				setCurrentStatusMessage("Client requested rect");

				if (message[1] == '\x00') {
					// "incremental" mode
					uint16_t xpos = (unsigned char)message[3] + ((unsigned char)message[2] * 256);
					uint16_t ypos = (unsigned char)message[5] + ((unsigned char)message[4] * 256);
					uint16_t wval = (unsigned char)message[7] + ((unsigned char)message[6] * 256);
					uint16_t hval = (unsigned char)message[9] + ((unsigned char)message[8] * 256);
					notifyClient_regionUpdated(xpos, ypos, wval, hval);
				} else {
					// await dirty-rect in this area
				}
				
			} else if (message.length() == 6 && message[0] == '\x05') {
				setCurrentStatusMessage("Client pointer event");
				uint8_t buttonMask = message[1];
				uint16_t xpos = (unsigned char)message[3] + ((unsigned char)message[2] * 256);
				uint16_t ypos = (unsigned char)message[5] + ((unsigned char)message[4] * 256);
				mouseEventRecieved(xpos, ypos, buttonMask);

			} else if (message.length() == 8 && message[0] == '\x04') {
				setCurrentStatusMessage("Keyboard event");

				uint32_t keysym = (unsigned char)message[7] + ((unsigned char)message[6] * 256) + ((unsigned char)message[5] * 65536) + ((unsigned char)message[4] * 16777216);
				if (message[1] == '\x00') {
					keyUpEventRecieved(keysym);
				} else {
					keyDownEventRecieved(keysym);
				}

			} else if (message.length() >= 4 && message[0] == '\x02') {
				supportedEncodings.clear();
				useEncodingMode = VEM_RAW;

				char* i = const_cast<char*>(message.c_str()) + 4;
				const char* e = message.c_str() + message.length();

				for (; i < e; i += 4) {
					uint32_t sval = ntohl(*(uint32_t*)(i));

					// The client sends in preference order
					if (useEncodingMode == VEM_RAW && (sval == VEM_ZLIB || sval == VEM_ZRLE || sval == VEM_TIGHTPNG)) {
						useEncodingMode = (enum VncdEncodingMode)sval;
					};

					supportedEncodings.push_back(sval);
				}

				if (useEncodingMode == VEM_RAW) {
					setCurrentStatusMessage("In RAW mode");

				} else if (useEncodingMode == VEM_ZRLE) {
					setCurrentStatusMessage("In ZRLE mode");

				}

			} else {
				setCurrentStatusMessage("Got message (unknown type)");

			}
		} break;

		default: {
			// No response was expected at this time
			setCurrentStatusMessage("Unexpected incoming message at this time");
			tcpConnection.close();
		} break;

	}

}

static std::string buildFramebufferUpdateMessage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t e) {
	std::string message;

	message.append("\x00\x00", 2); // FramebufferUpdate message

	uint16_t numRects = htons(1);
	message.append((char*)&numRects, sizeof(uint16_t));

	uint16_t
		xnet = htons(x),
		ynet = htons(y),
		wnet = htons(w),
		hnet = htons(h)
		;

	message.append((char*)&xnet, sizeof(uint16_t));
	message.append((char*)&ynet, sizeof(uint16_t));
	message.append((char*)&wnet, sizeof(uint16_t));
	message.append((char*)&hnet, sizeof(uint16_t));

	uint32_t encoding_type = htonl(e);
	message.append((char*)&encoding_type, sizeof(uint32_t));

	return message;
}

static void deflateString(mz_stream* mzs, std::string& in, std::string& out) {

	out.clear();
	unsigned long outBufferSize = compressBound(in.size());
	out.resize(outBufferSize, '\x00');

	size_t previously_written = mzs->total_out;

	mzs->avail_in = in.size();
	mzs->next_in = (unsigned char*)in.c_str();

	mzs->avail_out = outBufferSize;
	mzs->next_out = (unsigned char*)out.c_str();

	mz_deflate(mzs, MZ_SYNC_FLUSH);

	size_t written_now = mzs->total_out - previously_written;

	out.resize(written_now);
}

void VncdConnection::notifyClient_regionUpdated(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
	
#if 0

	// Some transmission types (e.g. TightPNG) have maximum rect sizes
	// Let's just never transmit anything bigger than 512x512.
	// n.b. This works on TightVNC Viewer, but fails on vnc4viewer! Disable for now.
	
	{
		if (w > VNCD_MAX_RECT_SIZE_WIDTH) {
			for (size_t xtmp = x; xtmp < x + w; xtmp += VNCD_MAX_RECT_SIZE_WIDTH) {
				notifyClient_regionUpdated(xtmp, y, std::min((size_t)VNCD_MAX_RECT_SIZE_WIDTH, w - xtmp), h);
			}
			return;
		}
		if (h > VNCD_MAX_RECT_SIZE_HEIGHT) {
			for (size_t ytmp = y; ytmp < y + h; ytmp += VNCD_MAX_RECT_SIZE_HEIGHT) {
				notifyClient_regionUpdated(x, ytmp, w, std::min((size_t)VNCD_MAX_RECT_SIZE_HEIGHT, h - ytmp));
			}
			return;
		}
	}

#endif

	setCurrentStatusMessage("Transmitting rect");

	//

	uint16_t framebufferWidth = getFrameWidth();
	uint16_t framebufferHeight = getFrameHeight();

	if (w == 0 || h == 0 || x + w > framebufferWidth || y + h > framebufferHeight) {
		setCurrentStatusMessage("Skipping out-of-bounds region update");
		return;
	}

	//

	std::string message = buildFramebufferUpdateMessage(x, y, w, h, useEncodingMode == VEM_TIGHTPNG ? VEM_TIGHT : useEncodingMode);

	//

	uint8_t* framebuffer = getFramebufferRGBX32();

	if (useEncodingMode == VEM_RAW) {

		std::string textBuffer;
		textBuffer.resize(w * h * (networkPixelFormat.bitsPerPixel / 8), '\x00');

		networkPixelFormat.copyRect(framebuffer, framebufferWidth, const_cast<char*>(textBuffer.c_str()), x, y, w, h);

		message.append(textBuffer);

		
	} else if (useEncodingMode == VEM_ZLIB) {

		std::string textBuffer;
		textBuffer.resize(w * h * (networkPixelFormat.bitsPerPixel / 8), '\x00');

		networkPixelFormat.copyRect(framebuffer, framebufferWidth, const_cast<char*>(textBuffer.c_str()), x, y, w, h);

		// Compress

		std::string compressedData;
		deflateString(&zlibStream, textBuffer, compressedData);

		// Tidy up

		textBuffer.clear(); // free slightly earlier

		// Add stream size to buffer

		uint32_t compressedData_network = htonl(compressedData.size());
		message.append((char*)&compressedData_network, sizeof(uint32_t));

		// Add stream to buffer

		message.append(compressedData);
		

	} else if (useEncodingMode == VEM_TIGHTPNG) {

		message.append("\x0A", 1); // png

		std::string textBuffer;
		textBuffer.resize(w * h * (networkPixelFormat.bitsPerPixel / 8), '\x00');

		networkPixelFormat.copyRect(framebuffer, framebufferWidth, const_cast<char*>(textBuffer.c_str()), x, y, w, h);

		size_t pngLen = 0;
		void* pngData = tdefl_write_image_to_png_file_in_memory(textBuffer.c_str(), w, h, 3, &pngLen);
		
		// Tight length format

		if (pngLen < 128) {
			uint8_t a = pngLen;
			message.append((char*)&a, 1);

		} else if (pngLen < 16384) {
			uint8_t a = (pngLen & 0x7F);
			uint8_t b = (pngLen >> 7) | 0x80;
			message.append((char*)b, 1);
			message.append((char*)a, 1);

		} else {
			uint8_t a = (pngLen & 0x7F);
			uint8_t b = (pngLen >> 7) | 0x80;
			uint8_t c = (pngLen >> 14) | 0x80;
			message.append((char*)c, 1);
			message.append((char*)b, 1);
			message.append((char*)a, 1);

		}
		
		message.append((const char*)pngData, pngLen);

		free(pngData); 


	} else if (useEncodingMode == VEM_ZRLE) {

		std::string compressedStream;
		std::string compressionBuffer;

		for (size_t tile_y = y; tile_y < (size_t)y + (size_t)h; tile_y += 64) {
			for (size_t tile_x = x; tile_x < (size_t)x + (size_t)w; tile_x += 64) {						

				// start at tile_x,tile_y ;; go to MIN(tile_x + 64, x+w),MIN(tile_y+64, y+h)

				size_t tile_y_max = std::min(tile_y + 64, (size_t)y + (size_t)h);
				size_t tile_x_max = std::min(tile_x + 64, (size_t)x + (size_t)w);
				size_t tile_width = tile_x_max - tile_x;
				size_t tile_height = tile_y_max - tile_y;

				// Read tile pixels

				std::string tileUncompressed;
				// tileUncompressed.append("\x00", 1); // added by next resize() call anyway
				tileUncompressed.resize(tile_width * tile_height * (networkPixelFormat.bitDepth / 8) + 1, '\x00');
				
				networkPixelFormat.copyRectCpixel(
					framebuffer, framebufferWidth, const_cast<char*>(tileUncompressed.c_str()) + 1,
					tile_x, tile_y, tile_width, tile_height
				);
				
				// Compress

				deflateString(&zrleStream, tileUncompressed, compressionBuffer);
				compressedStream.append(compressionBuffer);
			}
		}
		
		// Add stream size + stream to buffer

		uint32_t compressedData_network = htonl(compressedStream.size());
		message.append((char*)&compressedData_network, sizeof(uint32_t));
		message.append(compressedStream);
		
	}

	tcpConnection.async_send(
		asio::buffer(message.c_str(), message.length()),
		[this](std::error_code ec, std::size_t nb) {
			// do nothing
		}
	);
}

void VncdConnection::notifyClient_bell() {
	tcpConnection.async_send(
		asio::buffer("\x02", 1),
		[this](std::error_code ec, std::size_t nb) {
			// do nothing
		}
	);
}

void VncdConnection::notifyClient_sizeChanged() {

	for (uint32_t allowed : supportedEncodings) {
		if (allowed == -223) {

			// We can change the framebuffer size on the client

			std::string message = buildFramebufferUpdateMessage(0, 0, getFrameWidth(), getFrameHeight(), -223);

			tcpConnection.async_send(
				asio::buffer(message.c_str(), message.length()),
				[this](std::error_code ec, std::size_t nb) {
					// do nothing
				}
			);
			return;

		}
	}

}
