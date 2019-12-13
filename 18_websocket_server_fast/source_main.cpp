#include <iostream>
#include <exception>

#include "websocket_server_fast.hpp"

int main() {

	try {

		websocket_server_fast();
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << "\n";
		return EXIT_FAILURE;
	}
}