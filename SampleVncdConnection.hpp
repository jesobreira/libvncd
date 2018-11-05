/* SampleVncdConnection.hpp */

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

#include "VncdConnection.hpp"
#include "VncdTimer.hpp"

class SampleVncdConnection : public VncdConnection {

public:

	SampleVncdConnection(asio::ip::tcp::socket tcpConnection, VncdTimer timer) :
		VncdConnection(std::move(tcpConnection), std::move(timer))
	{
		memset(framebuffer, 0, sizeof(framebuffer));
		// noop
	}

	virtual ~SampleVncdConnection();

	uint8_t framebuffer[640 * 480 * 4];

	void fillFramebufferWith(uint8_t r, uint8_t g, uint8_t b);

#ifdef WIN32
	void fillFramebufferWithScreenshot();
#endif

protected:

	virtual void keyDownEventRecieved(uint32_t keysym);

	virtual void keyUpEventRecieved(uint32_t keysym);

	virtual void mouseEventRecieved(uint16_t xpos, uint16_t ypos, uint8_t buttonMask);

	virtual void connectionStarted();

	virtual uint8_t* getFramebufferRGBX32();

	virtual uint16_t getFrameWidth();

	virtual uint16_t getFrameHeight();

	virtual std::string getSessionTitle();

	virtual std::string requirePassword();

	virtual void setCurrentStatusMessage(std::string& msg);

	virtual void setCurrentStatusMessage(const char* msg);

};
