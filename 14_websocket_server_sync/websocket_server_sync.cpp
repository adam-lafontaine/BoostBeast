//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: WebSocket server, synchronous
//
//------------------------------------------------------------------------------

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

#include "websocket_server_sync.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

// Echoes back all received WebSocket messages
void do_session(tcp::socket& socket)
{
	try
	{
		// Construct the stream by moving in the socket
		websocket::stream<tcp::socket> ws{ std::move(socket) };

		// Set a decorator to change the Server of the handshake
		ws.set_option(websocket::stream_base::decorator(
			[](websocket::response_type& res)
			{
				res.set(http::field::server,
					std::string(BOOST_BEAST_VERSION_STRING) +
					" websocket-server-sync");
			}));

		// Accept the websocket handshake
		ws.accept();

		for (;;)
		{
			std::cout << "Waiting for a message...\n";

			// This buffer will hold the incoming message
			beast::flat_buffer buffer;

			// Read a message
			ws.read(buffer);

			// The make_printable() function helps print a ConstBufferSequence
			auto client_message = beast::make_printable(buffer.data());
			
			std::cout << "The client sent: " << client_message << "\n";

			std::ostringstream oss;

			oss << "Message received [" << client_message << "]\n";

			// Echo the message back
			ws.text(ws.got_text());
			//ws.write(buffer.data());
			ws.write(net::buffer(oss.str()));
		}
	}
	catch (beast::system_error const& se)
	{
		// This indicates that the session was closed
		if (se.code() != websocket::error::closed)
			std::cerr << "Error: " << se.code().message() << std::endl;
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
	}
}

//------------------------------------------------------------------------------

void websocket_server_sync()
{
	// Check command line arguments.
	/*if (argc != 3)
	{
		std::cerr <<
			"Usage: websocket-server-sync <address> <port>\n" <<
			"Example:\n" <<
			"    websocket-server-sync 0.0.0.0 8080\n";
		return EXIT_FAILURE;
	}
	auto const address = net::ip::make_address(argv[1]);
	auto const port = static_cast<unsigned short>(std::atoi(argv[2]));*/

	auto const address = net::ip::make_address("0.0.0.0");
	auto const port = 3000;

	// The io_context is required for all I/O
	net::io_context ioc{ 1 };

	// The acceptor receives incoming connections
	tcp::acceptor acceptor{ ioc, {address, port} };
	for (;;)
	{
		// This will receive the new connection
		tcp::socket socket{ ioc };

		std::cout << "Waiting for a connection...\n";

		// Block until we get a connection
		acceptor.accept(socket);

		// Launch the session, transferring ownership of the socket
		std::thread{ std::bind(
			&do_session,
			std::move(socket)) }.detach();
	}
}