#include <iostream>
#include <exception>

#include "websocket_server_stackless.hpp"

int main() {

	try {

		websocket_server_stackless();
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << "\n";
		return EXIT_FAILURE;
	}
}