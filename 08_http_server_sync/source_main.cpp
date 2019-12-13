#include <iostream>
#include <exception>

#include "http_server_sync.hpp"

int main() {
	char c;

	try {
		
		http_server_sync();
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << "\n";
		return EXIT_FAILURE;
	}
}