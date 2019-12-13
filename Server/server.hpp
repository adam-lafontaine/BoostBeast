#pragma once

#include <functional>

namespace server_async {	

	class Server {
	private:
		const char* address_;
		unsigned short port_;
		unsigned short threads_;

	public:
		Server(const char* address, unsigned short port, unsigned short num_threads)
			: address_(address), port_(port), threads_(num_threads) {}

		void start();

	};

	// "anonymous" types seen by user of server
	typedef struct request_type request;
	
	void send_text(
		request const& req,
		std::string const& body);
	
	void send_json(
		request const& req,
		std::string const& body);
	
	void send_file(
		request const& req,
		std::string const& file_path,
		const char* content_extension = "");

	using callback = std::function<void(request const&)>;

	void add_get(const char* target, callback const& func);
}

// https://docs.microsoft.com/en-us/cpp/build/walkthrough-creating-and-using-a-static-library-cpp?view=vs-2019