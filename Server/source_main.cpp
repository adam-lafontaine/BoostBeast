#include <iostream>
#include <exception>

#include "server.hpp"

namespace svr = server_async;

// C++17 for constexpr lambdas
auto constexpr send_text = 
[](auto const& req) { 
	svr::send_text(req, "Here is some text"); 
	std::cout << "send_text\n"; 
};

auto constexpr send_json = 
[](auto const& req) { 
	svr::send_json(req, "[{key: 'k1', value: 'v1'}, {key: 'k2', value: 'v2'}]"); 
	std::cout << "send_json\n"; 
};

auto constexpr send_file = 
[](auto const& req) { 
	svr::send_file(req, "./index.html"); 
	std::cout << "send_file\n"; 
};

void build_api() {
	svr::add_get("/text", send_text);
	svr::add_get("/json", send_json);
	svr::add_get("/file", send_file);
}

int main() {

	try {

		build_api();		

		auto address = "0.0.0.0";
		auto port = 5000;
		auto threads = 5;
		svr::Server server(address, port, threads);

		server.start();		
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << "\n";
		return EXIT_FAILURE;
	}
}