#include <iostream>
#include <exception>

#include "websocket_client_sync.hpp"

int main() {
	char c;

	try {
		while (true) {

			websocket_client_sync();


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