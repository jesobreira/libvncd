/* Vncd.hpp */

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
#include <string>
#include <list>
#include "asio_wrapper.h"

template <typename ConnectionAcceptor>
class Vncd {

public:

	asio::io_service io_service;

	Vncd() {
	}

	void acceptConnections(const char* bindTo, short port) {

		asio::ip::tcp::acceptor a(io_service, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port));

		asio::ip::tcp::socket sock(io_service);


		std::list<std::shared_ptr<ConnectionAcceptor>> activeClients;

		std::function<void(std::error_code)> accept_callback = [this, &a, &sock, &activeClients, &accept_callback](std::error_code err) {

			// First, let's clean up any closed sockets

			activeClients.remove_if( [](std::shared_ptr<ConnectionAcceptor>& client) {
				return ! client->tcpConnection.is_open();
			});

			// Process our new connection

			if (!err) {

				VncdTimer timer(io_service);

				std::shared_ptr<ConnectionAcceptor> handler = std::make_shared<ConnectionAcceptor>(std::move(sock), std::move(timer));
				activeClients.push_back(handler);
				handler->notifyClient_connectionAccepted();
			}
			
			// Loop

			a.async_accept(sock, accept_callback);
		};

		a.async_accept(sock, accept_callback);

		// Start main loop

		io_service.run();
	}

};
