/**
 * @file IrisRestfulNetworking.hpp
 * @author Ryan Landvater (ryanlandvater [at] gmail [dot] com)
 * @brief 
 * @version 0.1
 * @date 2025-06-07
 * 
 * @copyright Copyright (c) 2025 Iris Developers
 * 
 */

#ifndef IrisRestfulNetworking_hpp
#define IrisRestfulNetworking_hpp

namespace Iris {
namespace RESTful {
struct __INTERNAL__Session {
    const ASIOStream                    stream;
    const std::string                   remote;
    RESTful::Slide                      slide = nullptr;
    explicit __INTERNAL__Session        (ASIOSocket_t&&);
    __INTERNAL__Session                 (const __INTERNAL__Session&) = delete;
    __INTERNAL__Session& operator ==    (const __INTERNAL__Session&) = delete;
   ~__INTERNAL__Session                 ();
};
struct __INTERNAL__SslSession {
    const ASIOSslStream                 stream;
    const std::string                   remote;
    RESTful::Slide                      slide = nullptr;
    explicit __INTERNAL__SslSession     (ASIOSocket_t&&, SSLContext_t&);
    __INTERNAL__SslSession              (const __INTERNAL__SslSession&) = delete;
    __INTERNAL__SslSession& operator == (const __INTERNAL__SslSession&) = delete;
   ~__INTERNAL__SslSession              ();
};
class __INTERNAL__Networking {
    __INTERNAL__Server * const          _server;
    const Threads                       _reactors;
    const ASIOContext                   _context    = nullptr;
    const ASIOGuard                     _guard      = nullptr;
    const SSLContext                    _ssl        = nullptr;
    const Address                       _CORS       = "*";
    ASIOAcceptor                        _acceptor   = nullptr;
    
    atomic_bool                         ACTIVE;
public:
    explicit __INTERNAL__Networking     (__INTERNAL__Server* const &, bool https,
                                         const std::filesystem::path& cert_file,
                                         const std::filesystem::path& key_file,
                                         const Address& CORS);
    __INTERNAL__Networking              (const __INTERNAL__Networking&) = delete;
    __INTERNAL__Networking& operator == (const __INTERNAL__Networking&) = delete;
   ~__INTERNAL__Networking              ();
    void listen                         (uint16_t port);
    
private:
    void accept_connection              (const ASIOAcceptor&);
    
    template <class Session_>
    void read_request                   (const Session_&);
    
    template <class Session_>
    void interpret_request              (const Session_&, const HTTPRequest_t&);
    
    template <class Session_>
    void send_response                  (const Session_&, const HTTPResponse&);
    
    template <class Session_>
    void send_buffer                    (const Session_&, const HTTPResponseBuffer&, const Buffer&);
    
    template <class Session_>
    void send_file                      (const Session_&, const HTTPResponseFile&);
    
    template <class Session_>
    void close_stream                   (const Session_&);
};
} // END RESTUFL
} // END IRIS


#endif /* IrisRestfulNetworking_hpp */
