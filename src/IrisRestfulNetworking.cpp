/**
 * @file IrisRestfulNetworking.cpp
 * @author Ryan Landvater (ryanlandvater [at] gmail [dot] com)
 * @brief Based upon the work of Vinnie Falco and Boost/Beast
 * @version 0.1
 * @date 2025-06-07
 * 
 * @copyright Copyright (c) 2025 Iris Developers
 * 
 */

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif // __clang__
#include <boost/beast.hpp>          // Beast websocket protocol
#include <boost/asio/strand.hpp>    // Asio strand-style execution
#include <boost/asio/ssl.hpp>       // Asio openssl interface
#include <boost/exception/all.hpp>  // Boost runtime exceptions
#ifdef __clang__
#pragma clang diagnostic pop
#endif // __clang__

#define BOOST_IMPLEMENT // Allow for class definitions 
namespace   net       = boost::asio;
namespace   beast     = boost::beast;
namespace   ssl       = net::ssl;
namespace   ip        = net::ip;
using       tcp       = ip::tcp;
namespace   http      = beast::http;
#include "IrisRestfulPriv.hpp"

namespace Iris {
namespace RESTful {

// Forward declare SSL context creation. See IrisRestfulSSL.cpp
using fs_path = std::filesystem::path;
std::shared_ptr<boost::asio::ssl::context> CREATE_SSL_CONTEXT
 (const fs_path& cert_path, const fs_path& key_path);

// Define Session
inline std::string ADDRESS_TO_STRING (const tcp::endpoint& endpoint) {
    return endpoint.address().to_string()+":"+std::to_string(endpoint.port());
}
__INTERNAL__Session::__INTERNAL__Session(ASIOSocket_t&& socket, SSLContext_t& ctx) :
stream(std::make_unique<ASIOStream_t>(std::move(socket), ctx)),
remote(ADDRESS_TO_STRING(stream->lowest_layer().remote_endpoint()))
{
    std::cout <<"NEW SESSION\n";
}
__INTERNAL__Session::~__INTERNAL__Session() {
    std::cout <<"SESSION EXPIRED\n";
}

// Define Networking hub
__INTERNAL__Networking::__INTERNAL__Networking (const ServerCallbacks &cb,
                                                const fs_path& cert,
                                                const fs_path& key,
                                                const Address& CORS) :
_callbacks  (cb),
_reactors   (IRIS_CONCURRENCY),
_context    (std::make_shared<ASIOContext_t>(_reactors.size())),
_guard      (std::make_shared<ASIOGuard_t>(_context->get_executor())),
_ssl        (CREATE_SSL_CONTEXT(cert, key)),
_CORS       (CORS),
_acceptor   (nullptr),
ACTIVE      (true)
{
    if (!_ssl) throw std::runtime_error ("Failed to create SSL context");
    
    for (auto&& thread : const_cast<Threads&>(_reactors))
        thread = std::thread {[this](){
            // Run the context run loop within
            // a controlled try catch enviornment
            // for runtime exception recovery
            while (ACTIVE) try {
                _context->run();
            } catch (std::runtime_error& error) {
                std::cout   << "Network socket error: "
                            << error.what() << "\n";
            } catch (beast::error_code& error) {
                std::cout   << "Network socket error: "
                            << error.message() << "\n";
            } catch (...) {
                std::cout   << "Undefined network error\n";
            }
        }};
}
__INTERNAL__Networking::~__INTERNAL__Networking ()
{
    // Inactivate the context loops
    ACTIVE = false;
    
    // Interrupt the oustanding acceptor call
    _acceptor->cancel();
    
    // remove the execution guard
    const_cast<ASIOGuard&>(_guard) = nullptr;
    
    // Pump the event loop to unblock the reactor threads
    _context->poll();
    
    // And wait for all reactor threads to exit.
    for (auto&& thread : const_cast<Threads&>(_reactors)) {
        if (thread.joinable())
            thread.join();
    }
}
void __INTERNAL__Networking::listen(uint16_t port){
    if (_acceptor && _acceptor->is_open()) throw std::runtime_error
        ("networking acceptor already active");
    
    beast::error_code error;
    // NOTE: If set to IPv6, IPv4 connections will fire 2 acceptions (once for downgraded protocol)
    tcp::endpoint endpoint = tcp::endpoint(ip::tcp::v4(), port);
    
    // Create a new acceptor and give it a separate strand. It will pass the strand to the new socket.
    _acceptor = std::make_shared<ASIOAcceptor_t>(net::make_strand(_context->get_executor()));
    if (!_acceptor) throw std::runtime_error
        ("failed to create acceptor");
   
    // Open the acceptor
    _acceptor->open(endpoint.protocol(), error);
    if (error) throw std::runtime_error
        ("Failed to open acceptor: " + error.message() +
          "[FILE " + __FILE__ + "; LINE " + std::to_string(__LINE__) + "]");

    // Allow socket to be bound to a used address (maybe don't need to)
    _acceptor->set_option(net::socket_base::reuse_address(true), error);
    if (error) throw std::runtime_error
        ("Failed to set acceptor to reuse address option: " + error.message());

    // Bind to the server address
    _acceptor->bind(endpoint, error);
    if (error) throw std::runtime_error
        ("Failed to bind endpoint to acceptor: " + error.message() +
         "[FILE " + __FILE__ + "; LINE " + std::to_string(__LINE__) + "]");

    // Start listening for connections
    _acceptor->listen(net::socket_base::max_listen_connections, error);
    if (error) throw std::runtime_error
        ("Failed to listen with acceptor: " + error.message() +
         "[FILE " + __FILE__ + "; LINE " + std::to_string(__LINE__) + "]");
    
    // Report out to the console the local endpoint
    std::cout   << "[NOTE] Iris RESTful server is now listening at "
                << _acceptor->local_endpoint() << "\n";

    accept_connection(_acceptor);
}
void __INTERNAL__Networking::accept_connection(const ASIOAcceptor &acceptor)
{
    // Accept incoming connections
    // Note: If the sever is set to IPv6, this will fire twice upon a IPv4 request
    // Create a new strand; each acceptance carries the strand with the generated socket
    acceptor->async_accept(net::make_strand(acceptor->get_executor()),[this, acceptor]
                           (beast::error_code error, ASIOSocket_t socket){
        
        // Do not log an aborted operation; it means we are shutting the server down.
        if (error == net::error::operation_aborted) return;
        if (error) throw std::runtime_error
            ("Failed to accept an incomming connection: " + error.message());
        
        // Perpetuate the accept connection calls to keep the acceptor alive
        // If we have closed the acceptor, then gracefully exit and destroy acceptor
        if (acceptor->is_open()) accept_connection (acceptor);
        
        // Create a stream and begin reading messages
        auto session = std::make_shared<__INTERNAL__Session>(std::move(socket), *_ssl);
        beast::get_lowest_layer(*session->stream).expires_after(Time::seconds(30));
        (*session->stream).async_handshake(ssl::stream_base::server,[this,session]
                                           (beast::error_code error){
            if (error) {
                std::cerr   << "["<<session->stream->lowest_layer().remote_endpoint()<<"]"
                            << "Error in performing SSL handshake: "
                            << error.what() << "\n";
            } else {
                read_request (session);
            }
               
        });
    });
}
void __INTERNAL__Networking::read_request(const Session &session)
{
    // Extend the expiration time
    // This is ABSOLUTELY VITAL. Failure to do this will signficantly affect performance
    beast::get_lowest_layer(*session->stream).expires_after(Time::seconds(30));
    
    auto buffer  = std::make_shared<ASIOBuffer_t>();
    auto parser  = std::make_shared<http::request_parser<http::string_body>>();
    // This is a light-weight server; we don't expect big requests
    parser->header_limit(1024);
    parser->body_limit(2048);
    
    http::async_read(*session->stream, *buffer, *parser,
                     [this, session, buffer, parser]
                     (beast::error_code error, size_t bytes_transferred) {
        if (error) {
            if(error == http::error::end_of_stream ||
               error == beast::error::timeout)
                return close_stream (session);
            
            const auto response = std::make_shared<HTTPResponse_t>();
            response->version(11);
            response->set(http::field::content_type, "text/plain");
            response->keep_alive(parser->is_header_done()?parser->keep_alive():false);
            response->set(http::field::server, "IrisRESTful");
            if (error == http::error::header_limit) {
                // PROTECTION FROM DOS ATTACKS
                response->result(http::status::request_header_fields_too_large);
                response->body() = "IrisRESTful API HTTP header-length limit (1024) bytes exceeded";
                send_response (session, response);
            } else if (error == http::error::body_limit) {
                response->result(http::status::payload_too_large);
                response->body() = "IrisRESTful API payload-length limit (2048) bytes exceeded";
                send_response (session, response);
            } else {
                response->result(http::status::unknown);
                response->body() = "IrisRESTful API encountered undefined error: " + error.message();
                send_response(session, response);
            }
            return;
        }
        
        // Begin interpreting the request
        interpret_request(session, HTTPRequest_t(parser->release()));
    });
}
inline HTTPResponse GENERATE_STRING_GET_RESPONSE (const GetResponse &response) {
    HTTPResponse msg = std::make_shared<HTTPResponse_t>();
    switch (response.type) {
        case GetResponse::GET_RESPONSE_UNDEFINED:
        case GetResponse::GET_RESPONSE_MALFORMED_REQ:
            msg->result(http::status::bad_request);
            msg->set(http::field::content_type, "application/text");
            break;
        case GetResponse::GET_RESPONSE_FILE_NOT_FOUND:
            msg->result(http::status::not_found);
            msg->set(http::field::content_type, "application/text");
            break;
        case GetResponse::GET_RESPONSE_METADATA:
            msg->result(http::status::ok);
            msg->set(http::field::content_type, "application/json");
            break;
        case GetResponse::GET_RESPONSE_FILE:
        case GetResponse::GET_RESPONSE_TILE:
            goto MALFORMATTED_RESPONSE;
    }
    msg->body() = serialize_get_response(response);
    return msg;
    
    MALFORMATTED_RESPONSE:
    std::stringstream error;
    error   << "[ERROR] Cannot generate http::string_body for non-string response."
            <<" Set a breakpoint in " << __FILE__ << " line no " << __LINE__ << "to debug.";
    assert (false && error.str().c_str());
    throw std::runtime_error(error.str());
}
inline HTTPResponseFile GENERATE_FILE_RESPONSE (const GetFileResponse& file_response)
{
    beast::error_code ec;
    HTTPResponseFile msg  = std::make_shared<HTTPResponseFile_t>();
    msg->result(http::status::ok);
    msg->set(http::field::content_type, file_response.mime);
    msg->body().open(file_response.address.c_str(), beast::file_mode::read, ec);
    if (ec) {
        std::stringstream error;
        error   << "[ERROR] Cannot generate http::file_body response: " << ec.what() << ". "
                << "Set a breakpoint in " << __FILE__ << " line no " << __LINE__ << "to debug.";
        assert(false && error.str().c_str());
        throw std::runtime_error(ec.what());
    }
    return msg;
}
inline HTTPResponseBuffer GENERATE_TILE_RESPONSE (const GetTileResponse &tile_response)
{
    HTTPResponseBuffer msg  = std::make_shared<HTTPResponseBuffer_t>();
    msg->result(http::status::ok);
    msg->set(http::field::content_type, "image/jpeg");
    msg->body().data        = tile_response.pixelData->data();
    msg->body().size        = tile_response.pixelData->size();
    msg->body().more        = false;
    return msg;
}
// Generic Formatter Function. Applies generic server information to finalize response payloads.
template <class T>
inline void FORMAT_RESPONSE (http::response<T>& response, const HTTPRequest_t &request, const Address& CORS) {
    response.version(request.version());
    response.set(http::field::server, "Iris RESTful Server");
    if (CORS.length()) response.set("Access-Control-Allow-Origin", CORS);
    response.keep_alive(request.keep_alive());
    response.prepare_payload();
}
void __INTERNAL__Networking::interpret_request(const Session& session, const HTTPRequest_t &request)
{
    // The _callbacks.on_X_Request callbacks use a nested callback for a VERY good reason.
    // This allows the __INTERNAL__Server instance to push the implementation off the stack
    // to a separate worker thread and remove it from this ASIO/NetowrkingTS reactor thread.
    // I want to allow the ASIO threads to only work on the network queue.
    // The server request implementation functions on a completely disconnected stack / queue.
    // Do not mess with this design if you don't know what I'm talking about or without asking me.
    // - Ryan
    switch (request.method()) {
            
        // RESTful GET request
        case boost::beast::http::verb::head:
        case boost::beast::http::verb::get:
            // See __INTERNAL__Server::on_get_request (IrisRestfulServer.cpp) for implementation
            _callbacks.onGetRequest(session, std::string(request.target()),[this,session, request]
                                    (const std::unique_ptr<GetResponse>& response){
                
                switch (response->type) {
                    // Tile Data response (most frequent type of response)
                    case GetResponse::GET_RESPONSE_TILE: {
                        auto __response     = reinterpret_cast<GetTileResponse*>(response.get());
                        auto tile_response  = GENERATE_TILE_RESPONSE(*__response);
                        FORMAT_RESPONSE(*tile_response, request, _CORS);
                        return send_buffer(session, tile_response, __response->pixelData);
                    }
                        
                    // String / Text responses (returning text-formatted information)
                    case GetResponse::GET_RESPONSE_UNDEFINED:
                    case GetResponse::GET_RESPONSE_MALFORMED_REQ:
                    case GetResponse::GET_RESPONSE_FILE_NOT_FOUND:
                    case GetResponse::GET_RESPONSE_METADATA: {
                        auto string_response = GENERATE_STRING_GET_RESPONSE(*response);
                        FORMAT_RESPONSE(*string_response, request, _CORS);
                        return send_response(session, string_response);
                    }
                    
                    // File Server responses for Web server functionality (if enabled)
                    case GetResponse::GET_RESPONSE_FILE: {
                        auto __response     = reinterpret_cast<GetFileResponse*>(response.get());
                        auto file_response  = GENERATE_FILE_RESPONSE(*__response);
                        FORMAT_RESPONSE(*file_response, request, _CORS);
                        return send_file(session, file_response);
                    }
                        
                    
                }
            }); return;
        
        // RESTful POST request
        case http::verb::post:
        
        // RESTFUL PUT request
        case http::verb::put:
            
        // RESTFUL PATCH request
        case http::verb::patch:
            
        // RESTFUL DELETE request
        case http::verb::delete_:
        case http::verb::options:
        default: {
            
        } return;
        case http::verb::connect: {
            
        }
    }
}
void __INTERNAL__Networking::send_response(const Session &session, const HTTPResponse &response)
{
    http::async_write(*session->stream, *response,
                      [this, session, response]
                      (beast::error_code error, size_t bytes_transferred) {
        if (error) std::cerr    << "["<<session->remote<<"] "
                                << "Error writing response to stream: "
                                << error.message();
        if (response->keep_alive() && session->stream->lowest_layer().is_open())
                read_request (session);
        else    close_stream (session);
    });
}
void __INTERNAL__Networking::send_buffer(const Session &session, const HTTPResponseBuffer &response, const Buffer &buffer)
{
//    auto serializer = std::make_shared<http::serializer<false, http::buffer_body>> (*response);
    http::async_write(*session->stream, *response,
                      [this, session, response, buffer]
                      (beast::error_code error, size_t bytes_transferred) {
        if (error) std::cerr    << "["<<session->remote<<"] "
                                << "Error writing bin buffer response to stream: "
                                << error.message();
        if (response->keep_alive() && session->stream->lowest_layer().is_open())
                read_request (session);
        else    close_stream (session);
    });
}
void __INTERNAL__Networking::send_file(const Session &session, const HTTPResponseFile &response)
{
    http::async_write(*session->stream, *response,
                      [this, session, response]
                      (beast::error_code error, size_t bytes_transferred){
        if (error) std::cerr    << "["<<session->remote<<"] "
                                << "Error writing file response to stream: "
                                << error.message();
        if (response->keep_alive() && session->stream->lowest_layer().is_open())
                read_request (session);
        else    close_stream (session);
    });
}
void __INTERNAL__Networking::close_stream(const Session &session)
{
    beast::get_lowest_layer(*session->stream).expires_after(Time::seconds(30));
    beast::error_code error;
    if (session->stream->lowest_layer().is_open())
        (*session->stream).shutdown(error);
    if (error == net::ssl::error::stream_truncated ||
        error == net::error::broken_pipe);
    else if (error) std::cerr    << "["<<session->remote<<"] "
                            << "Error in closing the stream: "
                            << error.message() << "\n";
    
}
} // END RESTFUL
} // END IRIS
