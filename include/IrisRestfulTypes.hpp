/**
 * @file IrisRestfulTypes.hpp
 * @author Ryan Landvater (ryanlandvater [at] gmail [dot] com)
 * @brief Defines unique types used in this light HTTP/RESTful server 
 * for OpenSeaDragon based viewer implementations
 * that allows access to the Iris slide format (IFE). 
 * @version 0.1
 * @date 2025-06-07
 * 
 * @copyright Copyright (c) 2025 Iris Developers
 * 
 */

#include <filesystem>
#include "IrisCodecTypes.hpp"
#ifndef IrisRestfulTypes_hpp
#define IrisRestfulTypes_hpp
namespace Iris {
namespace RESTful {
namespace Time                      = std::chrono;
using Address                       = std::string;
using Port                          = uint16_t;
using Ports                         = std::vector<Port>;
using TimePoint                     = Time::time_point<Time::system_clock>;
using Server                        = std::shared_ptr<class __INTERNAL__Server>;

#ifdef BOOST_IMPLEMENT
// BOOST ASIO AND BEAST BOILERPLATE PREDEFINITIONS
// We use these predefinitions internally to avoid long compile times
using ASIOError_t                   = beast::error_code;
using ASIOContext_t                 = net::io_context;
using SSLContext_t                  = ssl::context;
using ASIOGuard_t                   = net::executor_work_guard<net::io_context::executor_type>;
using ASIOResolver_t                = ip::tcp::resolver;
using ASIOEndpoint_t                = ip::tcp::endpoint;
using ASIOAcceptor_t                = ip::tcp::acceptor;
using ASIOSocket_t                  = ip::tcp::socket;
using ASIOStream_t                  = ssl::stream<beast::tcp_stream>;
using ASIOBuffer_t                  = beast::flat_buffer;
using HTTPRequest_t                 = http::request<http::string_body>;
using HTTPResponse_t                = http::response<http::string_body>;
using HTTPResponseBuffer_t          = http::response<http::buffer_body>;
using HTTPRequestParser_t           = http::request_parser<http::string_body>;
#else
class ASIOError_t;
class ASIOContext_t;
class SSLContext_t;
class ASIOGuard_t;
class ASIOResolver_t;
class ASIOEndpoint_t;
class ASIOAcceptor_t;
class ASIOSocket_t;
class ASIOStream_t;
class ASIOFlatBuffer_t;
class HTTPRequest_t;
class ASIOBuffer_t;
class HTTPRequest_t;
class HTTPResponse_t;
class HTTPResponseBuffer_t;
class HTTPRequestParser_t;
#endif
class   __INTERNAL__Networking;
struct  __INTERNAL__Session;
class   __INTERNAL__Slide;
using ASIOContext                   = std::shared_ptr<ASIOContext_t>;
using SSLContext                    = std::shared_ptr<SSLContext_t>;
using ASIOGuard                     = std::shared_ptr<ASIOGuard_t>;
using ASIOResolver                  = std::shared_ptr<ASIOResolver_t>;
using ASIOEndpoint                  = std::shared_ptr<ASIOEndpoint_t>;
using ASIOAcceptor                  = std::shared_ptr<ASIOAcceptor_t>;
using ASIOStream                    = std::unique_ptr<ASIOStream_t>;
using ASIOBuffer                    = std::shared_ptr<ASIOBuffer_t>;
using HTTPRequest                   = std::shared_ptr<HTTPRequest_t>;
using HTTPResponse                  = std::shared_ptr<HTTPResponse_t>;
using HTTPResponseBuffer            = std::shared_ptr<HTTPResponseBuffer_t>;
using HTTPRequestParser             = std::shared_ptr<HTTPRequestParser_t>;
using Networking                    = std::unique_ptr<__INTERNAL__Networking>;
using Session                       = std::shared_ptr<__INTERNAL__Session>;
using Slide                         = std::shared_ptr<__INTERNAL__Slide>;
using SlideInfo                     = IrisCodec::SlideInfo;

struct ServerCreateInfo {
    std::filesystem::path slide_dir;
    std::filesystem::path cert;
    std::filesystem::path key;
};
struct GetRequest {
    enum Protocol {
        GET_REQUEST_MALFORMED       = 0,
        GET_REQUEST_IRIS,
        GET_REQUEST_DICOM,
    }           protocol            = GET_REQUEST_MALFORMED;
    enum Type {
        GET_REQUEST_UNDEFINED       = 0,
        GET_REQUEST_TILE,
        GET_REQUEST_METADATA,
//        GET_REQUEST_THUMBNAIL,
//        GET_REQUEST_SLIDE_LABEL,
    }           type                = GET_REQUEST_UNDEFINED;
    std::string error_msg;
    virtual ~GetRequest()           {}
};
struct GetTileRequest : public GetRequest {
    std::string id;
    uint32_t    layer               = 0;
    uint32_t    tile                = 0;
};
struct GetResponse {
    enum Type {
        GET_RESPONSE_UNDEFINED      = 0,
        GET_RESPONSE_MALFORMED_REQ,
        GET_RESPONSE_FILE_NOT_FOUND,
        GET_RESPONSE_TILE,
        GET_RESPONSE_METADATA,
    }           type                = GET_RESPONSE_UNDEFINED;
    bool        keep_alive          = false;
    std::string error_msg;
    virtual ~GetResponse(){}
};
struct PostResponse;
struct PutResponse;
struct GetTileResponse : public GetResponse {
    Buffer      pixelData           = nullptr;
    ~GetTileResponse()              {}
};
struct GetMetadataResponse : GetResponse {
    SlideInfo   slideInfo;
};
struct PostRequest;
struct PutRequest;

struct TileResponse {
    enum {
        RESPONSE_UNDEFINED,
        RESPONSE_SUCCESS,
        RESPONSE_UNKNOWN_FAILURE,
        RESPONSE_LAYER_OUT_OF_BOUNDS,
        RESPONSE_TILE_OUT_OF_BOUNDS,
    }           flag                = RESPONSE_UNDEFINED;
    Buffer      tile_data           = nullptr;
};
struct GetMetadataRequest : public GetRequest {
    std::string id;
    ~GetMetadataRequest()           {}
};
struct MetadataResponse {
    
};

// The ServerCallbacks use a nested callback for a VERY good reason.
// This allows the __INTERNAL__Server instance to push the implementation off the stack
// to separate worker threads and remove it from an ASIO/NetowrkingTS reactor thread.
// I want to reduce the reactor stack size and dedicate it to clearing the network queue.
// Do not mess with this design if you don't know what I'm talking about or without asking me.
// - Ryan
struct ServerCallbacks {
    std::function<void
    (const Session& session, const std::string& target,
     const std::function<void(const std::unique_ptr<GetResponse>&)>)>
    onGetRequest                    = nullptr;
    std::function<void
    (const Session& session, const std::string& target,
     const std::function<void(const std::unique_ptr<GetResponse>&)>)>
    onPostRequest                   = nullptr;
    std::function<void
    (const Session& session, const std::string& target,
     const std::function<void(const std::unique_ptr<GetResponse>&)>)>
    onPutRequest                    = nullptr;
};
} // END RESTFUL
} // END IRIS
#endif
