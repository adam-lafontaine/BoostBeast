#include <iostream>
#include <exception>

#include "http_server_stackless.hpp"

int main() {
	char c;

	try {

		http_server_stackless();
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << "\n";
		return EXIT_FAILURE;
	}
}