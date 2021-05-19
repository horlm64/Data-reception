#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
using response_type = http::response< http::string_body >;

// Report a failure
void fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
    std::cerr << "error code" << ": " << ec.value() << "\n";
    exit(-1);
}

// Sends a WebSocket message and prints the response
int main(int argc, char** argv)
{
    /*
    construction
    when a stream is constructed, any arguments provided to the constructors
    are forwarded to the next layer object's constructor
    this declares a stream over a plain tcp/ip socket using an I/O context
    A websocket stream object contains another stream object, next layer..
    which it uses to perform I/O
    */
    
    boost::system::error_code ec;
    // These objects perform our I/O
    net::io_context ioc;
    tcp::resolver resolver(ioc);
    websocket::stream<beast::tcp_stream> ws(ioc);

    // host / port / text
    std::string host = "192.168.1.1"; //   "echo.websocket.org"; 
    //std::string host = "echo.websocket.org"; 
    auto port = "80";
    auto const text = "Hello World! \n";
    //auto const text2 = "Hello Horim!";

    auto const results = resolver.resolve(host, port,ec);
    if (ec)
    {
        fail(ec, "resolve");
    }

    // Set a timeout on the operation
    beast::get_lowest_layer(ws).expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    auto ep = beast::get_lowest_layer(ws).connect(results, ec);
    if (ec)
    {
        fail(ec, "connect");
    }

    // Update the host_ string. This will provide the value of the
     // Host HTTP header during the WebSocket handshake.
     // See https://tools.ietf.org/html/rfc7230#section-5.4
    host += ':' + std::to_string(ep.port());

    // Turn off the timeout on the tcp_stream, because
    // the websocket stream has its own timeout system.
    beast::get_lowest_layer(ws).expires_never();
   
    // Set suggested timeout settings for the websocket
    ws.set_option(
        websocket::stream_base::timeout::suggested(
            beast::role_type::client));

    
    // Should be changed : Host field and the target of the resource to request
    // Set a decorator to change the User-Agent of the handshake
    ws.set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req)
        {
            std::unique_ptr<std::string> version = boost::make_unique<std::string>("11");
            req.set(http::field::version, *version);

        })); 
    //std::string(BOOST_BEAST_VERSION_STRING) +" websocket-client-coro"
  
    response_type res; // This variable will receive the HTTP response from the server
    ws.handshake(res,host, "/", ec);
    if (ec)
    {
        fail(ec, "handshake"); // bad version with error-code 14: The HTTP-version is invalid.
    }
    // This buffer will hold the incoming message
    beast::flat_buffer buffer;
    ws.write(net::buffer(std::string(text)),ec);
    if (ec)
    {
        fail(ec, "write");
    }
    ws.read(buffer, ec);
    if (ec)
    {
        fail(ec, "read");
    }
    // Close the WebSocket connection
    ws.close(websocket::close_code::normal);

    // The make_printable() function helps print a ConstBufferSequence
    std::cout << beast::make_printable(buffer.data()) << std::endl;
  

    return 0;

}
