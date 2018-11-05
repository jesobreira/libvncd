/* SampleVncdConnection.cpp */

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

#include "SampleVncdConnection.hpp"

#include <iostream>

#define XK_LATIN1
#include "X11/keysymdef.h"

void SampleVncdConnection::fillFramebufferWith(uint8_t r, uint8_t g, uint8_t b) {
	for (int y = 0; y < 480; ++y) {
	
		for (int x = 0; x < 640; ++x) {

			framebuffer[(x + (y * 640)) * 4 + 0] = r;
			framebuffer[(x + (y * 640)) * 4 + 1] = g;
			framebuffer[(x + (y * 640)) * 4 + 2] = b;

		}

	}
}

#ifdef WIN32

#include <Windows.h>
#include <wchar.h>

wchar_t seekWindowName[255];

void SampleVncdConnection::fillFramebufferWithWindow(HWND hWnd) {
	if (!hWnd) {
		setCurrentStatusMessage("Window not found");
		return;
	}

	RECT rect;
	GetWindowRect(hWnd, &rect);

	int nScreenWidth = rect.right - rect.left;
	int nScreenHeight = rect.bottom - rect.top;
	HDC hDesktopDC = GetWindowDC(hWnd);
	HDC hCaptureDC = CreateCompatibleDC(hDesktopDC);
	HBITMAP hCaptureBitmap = CreateCompatibleBitmap(hDesktopDC, nScreenWidth, nScreenHeight);
	SelectObject(hCaptureDC, hCaptureBitmap);

	BitBlt(hCaptureDC, 0, 0, nScreenWidth, nScreenHeight, hDesktopDC, 0, 0, SRCCOPY | CAPTUREBLT);

	BITMAPINFO bmi = { 0 };
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth = nScreenWidth;
	bmi.bmiHeader.biHeight = nScreenHeight;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	RGBQUAD *pPixels = new RGBQUAD[nScreenWidth * nScreenHeight];

	GetDIBits(
		hCaptureDC,
		hCaptureBitmap,
		0,
		nScreenHeight,
		pPixels,
		&bmi,
		DIB_RGB_COLORS
	);

	for (int x = 0; x < 640; ++x) {
		for (int y = 0; y < 480; ++y) {

			int src_index = (x + ((nScreenHeight - y) * nScreenWidth));
			int dest_index = (x + (y * 640)) * 4;

			if (src_index > nScreenWidth * nScreenHeight)
				src_index = nScreenWidth * nScreenHeight;

			framebuffer[dest_index + 0] = pPixels[src_index].rgbRed;
			framebuffer[dest_index + 1] = pPixels[src_index].rgbGreen;
			framebuffer[dest_index + 2] = pPixels[src_index].rgbBlue;

		}
	}


	delete[] pPixels;
}

void SampleVncdConnection::fillFramebufferWithScreenshot() {
	int nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	int nScreenHeight = GetSystemMetrics(SM_CYSCREEN);
	HWND hDesktopWnd = GetDesktopWindow();
	HDC hDesktopDC = GetDC(hDesktopWnd);
	HDC hCaptureDC = CreateCompatibleDC(hDesktopDC);
	HBITMAP hCaptureBitmap = CreateCompatibleBitmap(hDesktopDC, nScreenWidth, nScreenHeight);
	SelectObject(hCaptureDC, hCaptureBitmap);

	BitBlt(hCaptureDC, 0, 0, nScreenWidth, nScreenHeight, hDesktopDC, 0, 0, SRCCOPY | CAPTUREBLT);

	BITMAPINFO bmi = { 0 };
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth = nScreenWidth;
	bmi.bmiHeader.biHeight = nScreenHeight;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	RGBQUAD *pPixels = new RGBQUAD[nScreenWidth * nScreenHeight];

	GetDIBits(
		hCaptureDC,
		hCaptureBitmap,
		0,
		nScreenHeight,
		pPixels,
		&bmi,
		DIB_RGB_COLORS
	);

	for (int x = 0; x < 640; ++x) {
		for (int y = 0; y < 480; ++y) {

			int src_index = (x + ((nScreenHeight - y) * nScreenWidth));
			int dest_index = (x + (y * 640)) * 4;

			if (src_index > nScreenWidth * nScreenHeight)
				src_index = nScreenWidth * nScreenHeight;

			framebuffer[dest_index + 0] = pPixels[src_index].rgbRed;
			framebuffer[dest_index + 1] = pPixels[src_index].rgbGreen;
			framebuffer[dest_index + 2] = pPixels[src_index].rgbBlue;

		}
	}
	
	
	delete[] pPixels;
}

#endif

SampleVncdConnection::~SampleVncdConnection() {

}

void SampleVncdConnection::keyDownEventRecieved(uint32_t keysym) {
	
}

void SampleVncdConnection::keyUpEventRecieved(uint32_t keysym) {
	if (keysym == XK_r) {
		fillFramebufferWith(255, 0, 0);
		notifyClient_regionUpdated(0, 0, 640, 480);
	}

	else if (keysym == XK_g) {
		fillFramebufferWith(0, 255, 0);
		notifyClient_regionUpdated(0, 0, 640, 480);
	}

	else if (keysym == XK_b) {
		fillFramebufferWith(0, 0, 255);
		notifyClient_regionUpdated(0, 0, 640, 480);
	}

	else if (keysym == XK_z) {
		notifyClient_bell();

	}

#ifdef WIN32
	else if (keysym == XK_w) {
		fillFramebufferWithScreenshot();
		notifyClient_regionUpdated(0, 0, 640, 480);
	}

	else if (keysym = XK_y) {
		wchar_t* newSeekWindowName = L"Microsoft Visual Studio";
		std::copy_n(newSeekWindowName, wcslen(newSeekWindowName)+1, seekWindowName);
		EnumWindows(FindWindowPartial, reinterpret_cast<LPARAM>(this));
		notifyClient_regionUpdated(0, 0, 640, 480);
	}
#endif

	else if (keysym == XK_q) {
		notifyClient_sizeChanged();

	}
}

BOOL CALLBACK FindWindowPartial(HWND hwnd, LPARAM lParam) {
	wchar_t buffer[255];

	GetWindowText(hwnd, buffer, 255);

	// check if the window name partially matches the window we are looking for
	if (wcsstr(buffer, seekWindowName)) {
		reinterpret_cast<SampleVncdConnection*>(lParam)->fillFramebufferWithWindow(hwnd);
		return false;
	}
	else {
		return true;
	}
}

void SampleVncdConnection::mouseEventRecieved(uint16_t xpos, uint16_t ypos, uint8_t buttonMask) {

}

void SampleVncdConnection::connectionStarted() {
	// start timer

	fillFramebufferWith(255, 0, 0);
}

uint8_t* SampleVncdConnection::getFramebufferRGBX32() {
	return framebuffer;
}

uint16_t SampleVncdConnection::getFrameWidth() {
	return 640;
}

uint16_t SampleVncdConnection::getFrameHeight() {
	return 480;
}

std::string SampleVncdConnection::getSessionTitle() {
	return "VNCConnection";
}

void SampleVncdConnection::setCurrentStatusMessage(std::string& status) {
	std::cout << status << "\n";
}

void SampleVncdConnection::setCurrentStatusMessage(const char* status) {
	std::cout << status << "\n";
}

std::string SampleVncdConnection::requirePassword() {

	// Blank: no authentication required
	// Non-zero length: VNC authentication (DES)
	return "";
}
