#include <iostream>
#include <exception>

#include "websocket_server_async.hpp"

int main() {

	try {

		websocket_server_async();
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << "\n";
		return EXIT_FAILURE;
	}
}