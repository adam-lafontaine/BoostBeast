#include <algorithm>
#include <cstdlib>
#include <memory>
#include <thread>
#include <vector>
#include <unordered_map>

#include "server.hpp"
#include "server_util.hpp"

namespace sutil = server_util;

namespace server_async {

#define ADAM_VERSION_STRING "adam server"
	
	using beast_request = http::request<http::string_body>;	

	template<class Request, class Send>
	void handle_request(Request&& req, Send&& send);	

	//======= ERROR RESPONSES ===========================

	template<class Request, class Send>
	void send_bad_request(
		Request const& req,
		Send const& send,
		beast::string_view why)
	{
		http::response<http::string_body> res{ http::status::bad_request, req.version() };
		res.set(http::field::server, ADAM_VERSION_STRING);
		res.set(http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = std::string(why);
		res.prepare_payload();
		return send(std::move(res));
	}

	template<class Request, class Send>
	void send_not_found(
		Request const& req,
		Send const& send,
		beast::string_view resource)
	{
		http::response<http::string_body> res{ http::status::not_found, req.version() };
		res.set(http::field::server, ADAM_VERSION_STRING);
		res.set(http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = "The resource '" + std::string(resource) + "' was not found.";
		res.prepare_payload();
		return send(std::move(res));
	}

	template<class Request, class Send>
	void send_server_error(
		Request const& req,
		Send const& send,
		beast::string_view what)
	{
		http::response<http::string_body> res{ http::status::internal_server_error, req.version() };
		res.set(http::field::server, ADAM_VERSION_STRING);
		res.set(http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = "An error occurred: '" + std::string(what) + "'";
		res.prepare_payload();
		return send(std::move(res));
	}

	//======= SUCCESSFUL RESPONSES ========================

	template<class Request, class Send>
	void send_content(
		Request const& req,
		Send const& send,
		std::string const& body,
		const char* content_extension)
	{
		http::response<http::string_body> res{ http::status::not_found, req.version() };
		res.set(http::field::server, ADAM_VERSION_STRING);
		res.set(http::field::content_type, sutil::mime_type(content_extension));
		res.keep_alive(req.keep_alive());
		res.body() = body;
		res.prepare_payload();

		return send(std::move(res));
	}

	template<class Request, class Send>
	void send_file_t(
		Request const& req,
		Send const& send,
		std::string const& file_path,
		const char* content_extension)
	{
		beast::error_code ec;
		http::file_body::value_type body;
		body.open(file_path.c_str(), beast::file_mode::scan, ec);

		// Handle the case where the file doesn't exist
		if (ec == beast::errc::no_such_file_or_directory)
			return send_not_found(req, send, req.target());

		// Handle an unknown error
		if (ec)
			return send_server_error(req, send, ec.message());

		// Cache the size since we need it after the move
		auto const size = body.size();

		// Respond to GET request
		http::response<http::file_body> res{
			std::piecewise_construct,
			std::make_tuple(std::move(body)),
			std::make_tuple(http::status::ok, req.version())
		};

		res.set(http::field::server, ADAM_VERSION_STRING);

		// override content type for file extension
		if (strlen(content_extension) > 0)
			res.set(http::field::content_type, sutil::mime_type(content_extension));
		else
			res.set(http::field::content_type, sutil::mime_type(file_path));

		res.content_length(size);
		res.keep_alive(req.keep_alive());
		return send(std::move(res));
	}


	//======================================

	
	

	// Handles an HTTP server connection
	class session : public std::enable_shared_from_this<session>
	{
	public:
		// This is the C++11 equivalent of a generic lambda.
		// The function object is used to send an HTTP message.
		struct send_lambda
		{
			session& self_;

			explicit send_lambda(session& self) : self_(self) {}

			template<bool isRequest, class Body, class Fields>
			void operator()(http::message<isRequest, Body, Fields>&& msg) const
			{
				// The lifetime of the message has to extend
				// for the duration of the async operation so
				// we use a shared_ptr to manage it.
				auto sp = std::make_shared<
					http::message<isRequest, Body, Fields>>(std::move(msg));

				// Store a type-erased version of the shared
				// pointer in the class to keep it alive.
				self_.res_ = sp;

				// Write the response
				http::async_write(self_.stream_, *sp,
					beast::bind_front_handler(&session::on_write, self_.shared_from_this(), sp->need_eof()));
			}
		};

	private:

		beast::tcp_stream stream_;
		beast::flat_buffer buffer_;
		beast_request req_;
		std::shared_ptr<void> res_;
		send_lambda lambda_;

	public:
		// Take ownership of the stream
		session(tcp::socket&& socket)
			: stream_(std::move(socket)), lambda_(*this) {}

		// Start the asynchronous operation
		void run()
		{
			do_read();
		}

		void do_read()
		{
			// Make the request empty before reading,
			// otherwise the operation behavior is undefined.
			req_ = {};

			// Set the timeout.
			stream_.expires_after(std::chrono::seconds(30));

			// Read a request
			http::async_read(stream_, buffer_, req_,
				beast::bind_front_handler(&session::on_read, shared_from_this()));
		}

		void on_read(beast::error_code ec, std::size_t bytes_transferred)
		{
			boost::ignore_unused(bytes_transferred);

			// This means they closed the connection
			if (ec == http::error::end_of_stream)
				return do_close();

			if (ec)
				return sutil::fail(ec, "read");

			// Send the response
			handle_request(std::move(req_), lambda_);
		}

		void on_write(bool close, beast::error_code ec, std::size_t bytes_transferred)
		{
			boost::ignore_unused(bytes_transferred);

			if (ec)
				return sutil::fail(ec, "write");

			if (close)
			{
				// This means we should close the connection, usually because
				// the response indicated the "Connection: close" semantic.
				return do_close();
			}

			// We're done with the response so delete it
			res_ = nullptr;

			// Read another request
			do_read();
		}

		void do_close()
		{
			// Send a TCP shutdown
			beast::error_code ec;
			stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

			// At this point the connection is closed gracefully
		}
	};

	//------------------------------------------------------------------------------

	// Accepts incoming connections and launches the sessions
	class listener : public std::enable_shared_from_this<listener>
	{
		net::io_context& ioc_;
		tcp::acceptor acceptor_;

	public:
		listener(
			net::io_context& ioc,
			tcp::endpoint endpoint)
			: ioc_(ioc)
			, acceptor_(net::make_strand(ioc))
		{
			beast::error_code ec;

			// Open the acceptor
			acceptor_.open(endpoint.protocol(), ec);
			if (ec)
			{
				sutil::fail(ec, "open");
				return;
			}

			// Allow address reuse
			acceptor_.set_option(net::socket_base::reuse_address(true), ec);
			if (ec)
			{
				sutil::fail(ec, "set_option");
				return;
			}

			// Bind to the server address
			acceptor_.bind(endpoint, ec);
			if (ec)
			{
				sutil::fail(ec, "bind");
				return;
			}

			// Start listening for connections
			acceptor_.listen(net::socket_base::max_listen_connections, ec);
			if (ec)
			{
				sutil::fail(ec, "listen");
				return;
			}
		}

		// Start accepting incoming connections
		void run()
		{
			do_accept();
		}

	private:
		void do_accept()
		{
			// The new connection gets its own strand
			acceptor_.async_accept(net::make_strand(ioc_),
				beast::bind_front_handler(&listener::on_accept, shared_from_this()));
		}

		void on_accept(beast::error_code ec, tcp::socket socket)
		{	
			if (ec)
			{
				sutil::fail(ec, "accept");
			}
			else
			{
				// Create the session and run it
				std::make_shared<session>(std::move(socket))->run();
			}

			// Accept another connection
			do_accept();
		}
	};

	void Server::start() {
		auto const address = net::ip::make_address(address_);

		// The io_context is required for all I/O
		net::io_context ioc{ threads_ };

		// Create and launch a listening port
		std::make_shared<listener>(
			ioc,
			tcp::endpoint{ address, port_ })->run();

		// Run the I/O service on the requested number of threads
		std::vector<std::thread> v;
		v.reserve(threads_ - 1);
		for (auto i = threads_ - 1; i > 0; --i)
			v.emplace_back(
				[&ioc]
				{
					ioc.run();
				});

		ioc.run();
	}

	//==============================================

	// hide request and send types
	struct request_type {
		beast_request req;
		session::send_lambda send;
	};

	//==============================================

	std::unordered_map <std::string, callback> api_map_get
	{
		{ "/", [](auto const& req) { send_text(req, "Server is running"); }},
	};

	//----------------------------------

	// This function produces an HTTP response for the given
	// request. The type of the response object depends on the
	// contents of the request, so the interface requires the
	// caller to pass a generic lambda for receiving the response.
	template<class Request, class Send>
	void handle_request(Request&& req, Send&& send)
	{
		// Make sure we can handle the method
		if (req.method() != http::verb::get)
			return send_bad_request(req, send, "Unknown HTTP-method");
		
		if (req.target().empty() || req.target()[0] != '/')
			return send_bad_request(req, send, "Illegal request-target");

		auto key = std::string(req.target());

		if (api_map_get.find(key) == api_map_get.end()) // C++20 contains
			return send_bad_request(req, send, "Bad request-target");

		// call the requested function
		return api_map_get[key](request_type{ req, send });
	}


	//==== VISIBLE FUNCTIONS ===============================
	

	void send_text(
		request const& req,
		std::string const& body) {

		send_content(req.req, req.send, body, ".txt");
	}
	
	void send_json(
		request const& req,
		std::string const& body) {

		send_content(req.req, req.send, body, ".json");
	}

	void send_file(
		request const& req,
		std::string const& file_path,
		const char* content_extension) {

		send_file_t(req.req, req.send, file_path, content_extension);
	}

	// add a callback to the api
	void add_get(const char* target, callback const& func) {
		api_map_get[std::string(target)] = func;
	}

}

