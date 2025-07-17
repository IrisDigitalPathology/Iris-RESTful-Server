/**
 * @file IrisRestfulServer.cpp
 * @author Ryan Landvater (ryanlandvater [at] gmail [dot] com)
 * @brief
 * @version 0.1
 * @date 2025-06-07
 *
 * @copyright Copyright (c) 2025 Iris Developers
 *
 */
#include "IrisRestfulPriv.hpp"
Iris::RESTful::Server Iris::RESTful::create_server(const ServerCreateInfo& info)
{
    ServerCreateInfo mut_info = info;
    
    // Format the path string
    mut_info.slide_dir.make_preferred();
    if (mut_info.slide_dir.string().back() != std::filesystem::path::preferred_separator)
        mut_info.slide_dir+=std::filesystem::path::preferred_separator;
    
    if (std::filesystem::is_directory(mut_info.slide_dir) == false) {
        std::cerr   << "[ERROR] File system reports the provided slide root directory ("
                    << info.slide_dir
                    << ") does not exist\n";
        return nullptr;
    }
    
    // Format the cert path
    if (!mut_info.cert.empty()) {
        mut_info.cert.make_preferred();
        if (std::filesystem::exists(mut_info.cert) == false) {
            std::cerr   << "[ERROR] File system reports the provided certificate ("
                        << mut_info.cert
                        << ") does not exist\n";
            return nullptr;
        }
    }
    
    // Format the key path
    if (!mut_info.key.empty()) {
        mut_info.key.make_preferred();
        if (std::filesystem::exists(mut_info.key) == false) {
            std::cerr   << "[ERROR] File system reports the provided key ("
                        << mut_info.key
                        << ") does not exist\n";
        }
    }
    
    // Check the doc root if using for static file serving
    if (!mut_info.doc_root.empty()) {
        mut_info.slide_dir.make_preferred();
        if (mut_info.slide_dir.string().back() != std::filesystem::path::preferred_separator)
            mut_info.slide_dir+=std::filesystem::path::preferred_separator;
        
        if (std::filesystem::is_directory(mut_info.slide_dir) == false) {
            std::cerr   << "[ERROR] File system reports the provided document root directory ("
                        << info.slide_dir
                        << ") does not exist. Using IrisRESTful for static file serving is optional.\n";
            return nullptr;
        }
    }
    
    // Create a server instance
    try { return std::make_shared<__INTERNAL__Server>(mut_info); }
    catch (std::runtime_error& error) {
        std::string msg = error.what() ? error.what() :
        std::string("[undefined error in file") + __FILE__ + "]";
        std::cerr   << "Failed to create server instance: "
                    << msg << "\n";
        return NULL;
    }
    
}

Iris::Result Iris::RESTful::server_listen(const Server& server, uint16_t port)
{
    try {
        // Check the server is valid
        if (!server) throw std::runtime_error("Invalid server object provided");
        
        // Begin listening at the designated port
        server->listen(port);
        
    } catch (std::runtime_error& error) {
        std::string msg = error.what() ? error.what() :
        std::string("[undefined error in file") + __FILE__ + "]";
        return Result (IRIS_FAILURE,
                       std::string("Iris RESTful Server failed to listen at ") +
                       std::to_string(port) + ". " + msg);
        
    }   return IRIS_SUCCESS;
}

namespace Iris {
namespace RESTful {
using namespace std::placeholders;
__INTERNAL__Server::__INTERNAL__Server(const ServerCreateInfo& info) :
_root       (info.slide_dir),
_doc_root   (info.doc_root),
_networking (std::make_unique<__INTERNAL__Networking>(ServerCallbacks {
    .onGetRequest = std::bind(&__INTERNAL__Server::on_get_request, this, _1, _2, _3),
}, info.cert, info.key, info.cors.length()?info.cors:_doc_root.empty()?"*":"")),
// ^Assign a designated CORS, if empty assign * only if no webserver root.
_threads(Async::createThreadPool())
{
    
}
void __INTERNAL__Server::listen(uint16_t port)
{
    _networking->listen(port);
}
inline std::unique_ptr<GetResponse> PROCESS_GET_FILE_REQUEST (const std::unique_ptr<GetRequest> &_r, const std::filesystem::path& doc_root)
{
    assert(_r->protocol == GetRequest::GET_REQUEST_FILE && "PROCESS_GET_FILE_REQUEST attempting to interpret GetRequest of invalid type (not GetRequest::GET_REQUEST_FILE)");
    assert(doc_root.empty() == false && "PROCESS_GET_FILE_REQUEST attempting to file serve non-web-server configured Iris RESTful.");
    
    const auto& request     = *reinterpret_cast<GetFileRequest*>(_r.get());
    auto response           = std::make_unique<GetFileResponse>();
    try {
        auto path = std::filesystem::path(doc_root.string() + request.path).make_preferred();
        if (std::filesystem::exists(path) == false) throw std::runtime_error
            ("File '" + request.path + "' not found");
        response->type      = GetResponse::GET_RESPONSE_FILE;
        response->address   = path;
        response->mime      = request.mime;
    } catch (std::runtime_error& e) {
        response->type      = GetResponse::GET_RESPONSE_FILE_NOT_FOUND;
        response->error_msg = e.what()?e.what():"[undefined error]";
    }
    return response;
}
inline std::unique_ptr<GetResponse> PROCESS_GET_TILE_REQUEST (const std::unique_ptr<GetRequest> &_r, const Slide &slide)
{
    assert(_r->type == GetRequest::GET_REQUEST_TILE && "PROCESS_GET_TILE_REQUEST attempting to interpret GetRequest of invalid type (not GetRequest::GET_REQUEST_TILE)");
    assert(slide && "PROCESS_GET_TILE_REQUEST attempting to interpret GetRequest with invalid slide handle.");
    
    const auto& request     = *reinterpret_cast<GetTileRequest*>(_r.get());
    auto response           = std::make_unique<GetTileResponse>();
    try {
        if (!slide) throw std::runtime_error ("No valid slide file found");
        response->type      = GetResponse::GET_RESPONSE_TILE;
        response->pixelData = slide->get_tile_entry(request.layer, request.tile);
    } catch (std::runtime_error& e) {
        response->type      = GetResponse::GET_RESPONSE_FILE_NOT_FOUND;
        response->error_msg = e.what();
    }
    return response;
}
inline std::unique_ptr<GetResponse> PROCESS_GET_METATADATA_REQUEST (const std::unique_ptr<GetRequest> &_r, const Slide &slide)
{
    assert(_r->type == GetRequest::GET_REQUEST_METADATA && "PROCESS_GET_METATADATA_REQUEST attempting to interpret GetRequest of invalid type (not GetRequest::GET_REQUEST_METADATA)");
    assert(slide && "PROCESS_GET_METATADATA_REQUEST attempting to interpret GetRequest with invalid slide handle.");
    
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused"
    // Ignore for now. May need it later...
    const auto& request = *reinterpret_cast<GetMetadataRequest*>(_r.get());
    #pragma clang diagnostic pop
    
    auto response       = std::make_unique<GetMetadataResponse>();
    try {
        if (!slide) throw std::runtime_error ("No valid slide file found");
        response->type      = GetResponse::GET_RESPONSE_METADATA;
        response->slideInfo = slide->get_slide_info();
    } catch (std::runtime_error& e) {
        response->type  = GetResponse::GET_RESPONSE_FILE_NOT_FOUND;
        response->error_msg = e.what();
    }
    return response;
}
inline std::unique_ptr<GetResponse> INVALID_SLIDE_IDENTIFIER (std::string& identifier)
{
    auto response       = std::make_unique<GetMetadataResponse>();
    response->type  = GetResponse::GET_RESPONSE_FILE_NOT_FOUND;
    response->error_msg = "Slide file with identifier '" + identifier + "' not found.";
    return response;
}
Slide __INTERNAL__Server::get_slide (const std::string &id)
{
    // Let's see if the slide is already open
    // Look it up in the directory
    ReadLock read_lock (_directory.mutex);
    auto __slide = _directory.find(id);
    if (__slide != _directory.cend()) if
        (Slide slide = __slide->second.lock())
            return slide;
    read_lock.unlock();
    
    // The slide was not found.
    // We will open a new slide instead.
    // Create the slide
    
    std::filesystem::path file_path (_root.string()+id+".iris");
    Slide slide;
    try { slide = validate_and_open_slide(file_path);}
    catch (std::runtime_error &error) {
        std::string msg = error.what() ? error.what() :
        std::string("[undefined error in file") + __FILE__ + "]";
        std::cerr   << "Failed to open slide id ("
                    <<id<<"): "<<msg<<"\n";
        return nullptr;
    }
    
    // Gain exclusive access to the slide directory and check that
    // a competing request / thread did not just create one as well
    ExclusiveLock update_lock (_directory.mutex);
    __slide = _directory.find(id);
    if (__slide != _directory.cend()) if
        (Slide just_made = __slide->second.lock())
        return just_made;
    
    // An interviening slide was not made (still no slide there)
    // so add the just made slide and unlock the exclusive.
    _directory[id] = slide;
    update_lock.unlock();
    
    // Add a callback to remove the slide file from the server directory
    // when it expires.
    slide->set_on_destroyed_callback([this, id]() {
        // Check to see that this is still in the directory
        ReadLock read_lock (_directory.mutex);
        auto __slide = _directory.find(id);
        if  (__slide == _directory.cend()) return;
        read_lock.unlock();
        
        // If it is found, remove it, if this is the entry.
        ExclusiveLock update_lock (_directory.mutex);
        if (__slide->second.expired()) _directory.erase(__slide);
    });
    return slide;
}
void __INTERNAL__Server::on_get_request(const Session& session,
                                        const std::string& target,
                                        const std::function<void(const std::unique_ptr<GetResponse>&)> on_response)
{
    // Push the processing of requests off the network stack onto the
    // server's response stack. This confines the activities of the io_context
    // reactor threads (controlled by NetworkingTS/ASIO) only to networking tasks.
    _threads->issue_task([this, session, target, on_response](){
        // Parse the get request target sequence
        auto request    = parse_get_request (target);
        auto response   = std::make_unique<GetResponse>();
        
        // Ensure it follows a supported RESTful API
        //  -- Currently that's IrisRESTful and WADO-RS
        //  -- OPTIONALLY that includes a webserver / file server
        switch (request->protocol) {
            // Are we attempting to use Iris RESTFUL as a webserver as well
            // to avoid Cross Origin serving?
            case GetRequest::GET_REQUEST_FILE:/* OPTIONAL must be activated */
                if(_doc_root.empty()) {
                    response->type = GetMetadataResponse::GET_RESPONSE_FILE_NOT_FOUND;
                    response->error_msg =
                    "This Iris RESTful implementation is not configured to run as a web server / file server.";
                    return on_response(response);
                } else return on_response(PROCESS_GET_FILE_REQUEST(request, _doc_root));
                
                
            // Standard IRIS or DICOM Requests
            case GetRequest::GET_REQUEST_IRIS:  break;
            case GetRequest::GET_REQUEST_DICOM: break;
            
            // Anything else is considered malformed
            case GetRequest::GET_REQUEST_MALFORMED:
            default: goto MALFORMED_REQUEST;
        }
        
        // Check to see what is requested
        switch (request->type) {

            case GetRequest::GET_REQUEST_TILE: {
                auto& __request = *reinterpret_cast<GetTileRequest*>(request.get());
                if (!session->slide || *(session->slide) != __request.id) {
                    session->slide = get_slide(__request.id);
                    if (!session->slide) {
                        on_response(INVALID_SLIDE_IDENTIFIER(__request.id));
                        return;
                    }
                }
                on_response(PROCESS_GET_TILE_REQUEST(request, session->slide));
                return;
            }
                
                
            case GetRequest::GET_REQUEST_METADATA: {
                auto& __request = *reinterpret_cast<GetTileRequest*>(request.get());
                if (!session->slide || *(session->slide) != __request.id) {
                    session->slide = get_slide(__request.id);
                    if (!session->slide) {
                        on_response(INVALID_SLIDE_IDENTIFIER(__request.id));
                        return;
                    }
                }
                on_response(PROCESS_GET_METATADATA_REQUEST(request, session->slide));
                return;
            }
                
            case GetRequest::GET_REQUEST_UNDEFINED:
            default: goto MALFORMED_REQUEST;
        }
        
        MALFORMED_REQUEST:
        response->type  = GetResponse::GET_RESPONSE_MALFORMED_REQ;
        response->error_msg = request->error_msg.size()?request->error_msg:
        "Undefined GET request error. IrisRESTful server did elaborate on what happened.";
        on_response (response);
    });
}
} // END RESTFUL
} // END IRIS
