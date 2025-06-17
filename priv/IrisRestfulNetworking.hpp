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
    RESTful::Slide                      slide = nullptr;
    explicit __INTERNAL__Session        (ASIOSocket_t&&);
    __INTERNAL__Session                 (const __INTERNAL__Session&) = delete;
    __INTERNAL__Session& operator ==    (const __INTERNAL__Session&) = delete;
   ~__INTERNAL__Session                 ();
};
class __INTERNAL__Networking {
    const ServerCallbacks               _callbacks;
    const Threads                       _reactors;
    const ASIOContext                   _context     = nullptr;
    const ASIOGuard                     _guard       = nullptr;
    ASIOAcceptor                        _acceptor    = nullptr;
    
    atomic_bool                         ACTIVE;
public:
    explicit __INTERNAL__Networking     (const ServerCallbacks&);
    __INTERNAL__Networking              (const __INTERNAL__Networking&) = delete;
    __INTERNAL__Networking& operator == (const __INTERNAL__Networking&) = delete;
   ~__INTERNAL__Networking              ();
    void listen                         (uint16_t port);
    
private:
    void accept_connection              (const ASIOAcceptor&);
    void read_request                   (const Session&);
    void interpret_request              (const Session&, const HTTPRequest_t&);
    void send_response                  (const Session&, const HTTPResponse&);
    void send_buffer                    (const Session&, const HTTPResponseBuffer&, const Buffer&);
    void close_stream                   (const Session&);
};
} // END RESTUFL
} // END IRIS


#endif /* IrisRestfulNetworking_hpp */
