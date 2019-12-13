#include <iostream>
#include <exception>

#include "websocket_server_coro.hpp"

int main() {

	try {

		websocket_server_coro();
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << "\n";
		return EXIT_FAILURE;
	}
}