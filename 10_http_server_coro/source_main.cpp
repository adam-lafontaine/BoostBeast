#include <iostream>
#include <exception>

#include "http_server_coro.hpp"

int main() {
	char c;

	try {

		http_server_coro();
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << "\n";
		return EXIT_FAILURE;
	}
}