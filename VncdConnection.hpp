/* VncdConnection */

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
#include <functional>
#include <memory>
#include <cstdint>
#include <chrono>
#include <vector>
#include "asio_wrapper.h"
#include "asio/asio/detail/noncopyable.hpp"
#include "miniz_wrapper.h"
#include "RFBPixelFormat.hpp"
#include "VncdTimer.hpp"

enum VncdConnectionState {
	VCS_INVALID = 0,

	VCS_HANDSHAKE_1_SENDING_PROTOCOL,
	VCS_HANDSHAKE_2_WAITING_FOR_PROTOCOL_RESPONSE,
	VCS_HANDSHAKE_3_SENDING_SECURITY_TYPES,
	VCS_HANDSHAKE_4_WAITING_FOR_SECURITY_SELECTION,
	VCS_HANDSHAKE_5_SENDING_SECURITY_CHALLENGE,
	VCS_HANDSHAKE_6_WAITING_FOR_SECURITY_RESPONSE,
	VCS_HANDSHAKE_7_SENDING_SECURITY_OK,
	VCS_HANDSHAKE_8_WAITING_FOR_CLIENTINIT,
	VCS_HANDSHAKE_9_SENDING_SERVERINIT,

	VCS_READY
};

enum VncdEncodingMode {
	VEM_RAW = 0,
	VEM_ZLIB = 6,
	VEM_TIGHT = 7,
	VEM_ZRLE = 16,
	VEM_TIGHTPNG = -260
};

enum VncdMouseButtonMask {
	VMBM_LEFT		= 1 << 0,
	VMBM_MIDDLE		= 1 << 1,
	VMBM_RIGHT		= 1 << 2,
	VMBM_WHEELUP	= 1 << 3,
	VMBM_WHEELDOWN	= 1 << 4
};

class VncdConnection : public asio::noncopyable {

/* IMPLEMENTATION SHARED FOR ALL CHILD CLASSES */

public:
	
	VncdConnection(asio::ip::tcp::socket tcpConnection, VncdTimer timer);

	void notifyClient_connectionAccepted(); // begins main processing

	void notifyClient_sizeChanged();

	void notifyClient_regionUpdated(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

	void notifyClient_bell();

	asio::ip::tcp::socket tcpConnection;

	VncdTimer timer;
	
protected:

	RFBPixelFormat networkPixelFormat;

	asio::streambuf sb;

	asio::streambuf::mutable_buffers_type sb_mutable;

	std::string getBuffer(size_t numBytes);

	std::vector<uint32_t> supportedEncodings;

	uint32_t useEncodingMode;

	VncdConnectionState currentState;

	mz_stream zrleStream;
	mz_stream zlibStream;

	std::string desChallengeNonce;

	void awaitProtocolMessage();

	void handleProtocolMessage(std::string message);

/* TO BE OVERWRITTEN BY CHILD CLASSES */
	
public:

	virtual ~VncdConnection();

protected:

	virtual void keyDownEventRecieved(uint32_t keysym) = 0;

	virtual void keyUpEventRecieved(uint32_t keysym) = 0;

	virtual void mouseEventRecieved(uint16_t xpos, uint16_t ypos, uint8_t buttonMask) = 0;
	
	virtual void connectionStarted() = 0;

	virtual uint8_t* getFramebufferRGBX32() = 0;

	virtual uint16_t getFrameWidth() = 0;

	virtual uint16_t getFrameHeight() = 0;

	virtual std::string getSessionTitle() = 0;

	virtual std::string requirePassword() = 0;

	virtual void setCurrentStatusMessage(std::string& msg) = 0;

	virtual void setCurrentStatusMessage(const char* msg) = 0;

};
