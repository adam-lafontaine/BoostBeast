#include <iostream>
#include <exception>

#include "websocket_server_sync.hpp"

int main() {
	char c;

	try {

		websocket_server_sync();
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << "\n";
		return EXIT_FAILURE;
	}
}