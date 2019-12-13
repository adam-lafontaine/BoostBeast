#include <iostream>
#include <exception>

#include "http_client_sync.hpp"

int main() {
	char c;

	try {
		while (true) {

			http_client_sync();


			c = getchar();
			if (c == 'x')
				break;
		}
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << "\n";
		return EXIT_FAILURE;
	}
}