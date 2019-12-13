#include <iostream>
#include <exception>

#include "http_crawl.hpp"

int main() {
	char c;

	try {
		http_crawl();
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << "\n";
		return EXIT_FAILURE;
	}
}