/**
 * @file IrisRestfulGetParser.cpp
 * @author Ryan Landvater (ryanlandvater [at] gmail [dot] com)
 * @brief 
 * @version 0.1
 * @date 2025-06-07
 * 
 * @copyright Copyright (c) 2025 Iris Developers
 * 
 */

#include <cstring>
#include <charconv>
#include "IrisRestfulPriv.hpp"

namespace Iris {
namespace RESTful {

constexpr char target_delin = '/';
inline std::string_view PARSE_FRONT_TOKEN (const char*& ptr, const char* const end)
{
    // This is basically a safe version of strtok; updates ptr loc as it goes
    // so that's where the statefulness is.
    const char* const front = ++ptr; // front skips the delineator
    unsigned count = 0;
    for (;ptr < end+1 && *ptr != target_delin; ++ptr,++count);
    return std::string_view(front, count);
}
inline std::string_view PARSE_BACK_TOKEN (const char* const front, const char* const end)
{
    // NON stateful; this will NOT change front or end (unlike PARSE_FRONT_TOKEN above)
    // This just lets you sample the URL end to see if there's a file or command there.
    const char* r_it = end;
    unsigned count = 0;
    for (;r_it != front && *r_it != target_delin; --r_it,++count);
    return std::string_view(r_it+1, count);
}
inline bool PARSE_MIME (const char* const front, const char* const end, std::string* mime)
{
    auto token = PARSE_BACK_TOKEN(front, end);
    const char* r_it = token.end();
    unsigned count = 0;
    for (;r_it != token.data() && *r_it != '.'; --r_it,++count);
    
    // A file was not indicated
    if (r_it == token.data()) return false;
    
    auto mime_view = std::string_view (r_it,count);
    if (!mime_view.compare(".htm")) {if(mime)*mime="text/html";}
    else if (!mime_view.compare(".html")) {if(mime)*mime="text/html";}
    else if (!mime_view.compare(".php")) {if(mime)*mime="text/html";}
    else if (!mime_view.compare(".css")) {if(mime)*mime="text/css";}
    else if (!mime_view.compare(".txt")) {if(mime)*mime="text/plain";}
    else if (!mime_view.compare(".js")) {if(mime)*mime="application/javascript";}
    else if (!mime_view.compare(".json")) {if(mime)*mime="application/json";}
    else if (!mime_view.compare(".xml")) {if(mime)*mime="application/xml";}
    else if (!mime_view.compare(".dzi")) {if(mime)*mime="image/dzi";}
    else if (!mime_view.compare(".png")) {if(mime)*mime="image/png";}
    else if (!mime_view.compare(".jpe")) {if(mime)*mime="image/jpeg";}
    else if (!mime_view.compare(".jpeg")) {if(mime)*mime="image/jpeg";}
    else if (!mime_view.compare(".jpg")) {if(mime)*mime="image/jpeg";}
    else if (!mime_view.compare(".gif")) {if(mime)*mime="image/gif";}
    else if (!mime_view.compare(".bmp")) {if(mime)*mime="image/bmp";}
    else if (!mime_view.compare(".ico")) {if(mime)*mime="image/vnd.microsoft.icon";}
    else if (!mime_view.compare(".tiff")) {if(mime)*mime="image/tiff";}
    else if (!mime_view.compare(".tif")) {if(mime)*mime="image/tiff";}
    else if (!mime_view.compare(".svg")) {if(mime)*mime="image/svg+xml";}
    else if (!mime_view.compare(".svgz")) {if(mime)*mime="image/svg+xml";}
    else return false;
    return true;
}
inline GetRequest::Protocol PARSE_PROTOCOL (const char* loc, const char* const end)
{
    auto token = PARSE_FRONT_TOKEN(loc, end);
    for (; token.size() == 0 && loc < end+1;
         token = PARSE_FRONT_TOKEN(loc, end));
    if (token.compare("slides") == 0)
        return GetRequest::GET_REQUEST_IRIS;
    else if (token.compare("studies") == 0)
        return GetRequest::GET_REQUEST_DICOM;
    else if (PARSE_MIME(loc, end, NULL))
        return GetRequest::GET_REQUEST_FILE;
    else return GetRequest::GET_REQUEST_MALFORMED;
}
inline GetRequest::Type PARSE_COMMAND (const char* const front, const char* const end)
{
    auto back_token = PARSE_BACK_TOKEN(front, end);
    if (std::isdigit(*end))
        return GetRequest::GET_REQUEST_TILE;
    else if (back_token.compare("metadata") == 0)
        return GetRequest::GET_REQUEST_METADATA;
    else if (back_token.compare("thumbnail") == 0)
        assert(false&&"NOT BUILT YET");
    else if (back_token.compare("slide_label") == 0)
        assert(false&&"NOT BUILT YET");
    else if (back_token.compare("rendered") == 0)
        assert(false&&"NOT BUILT  YET");
    
    return GetRequest::GET_REQUEST_UNDEFINED;
}
inline std::unique_ptr<GetRequest> PARSE_IRIS_REQUEST (const char* loc, const char* const end) {
    std::string error_string;
    if (PARSE_FRONT_TOKEN(loc, end).compare("slides"))
        throw std::runtime_error(std::string("PARSE_IRIS_REQUEST called on non-IrisRESTful API GET request. Go to file ") +
                                 __FILE__ + " line " +std::to_string(__LINE__) + " to debug.");
    
    switch (PARSE_COMMAND(loc, end)) {
        case GetRequest::GET_REQUEST_UNDEFINED:
            error_string = "Undefined command sequence (last token) in IrisRESTful target URL. Please ensure your command conforms to the IrisRestful API.";
            goto MALFORMED_IRIS_REQUEST;
            
        case GetRequest::GET_REQUEST_TILE: {
            auto request = std::make_unique<GetTileRequest>();
            request->protocol   = GetRequest::GET_REQUEST_IRIS;
            request->type       = GetRequest::GET_REQUEST_TILE;
            request->id         = PARSE_FRONT_TOKEN(loc, end);
            auto layer_str      = PARSE_FRONT_TOKEN(loc, end);
            if (layer_str.compare("layers")){ //PARSE_FRONT_TOKEN(loc, end).compare("layers")) {
                error_string = "Expected 'layers' following slide identifier in IrisRESTful GET tile command target URL";
                goto MALFORMED_IRIS_REQUEST;
            }
            auto layer          = PARSE_FRONT_TOKEN(loc, end);
            auto result         = std::from_chars (layer.data(), layer.data()+layer.size(), request->layer);
            if (result.ec == std::errc::invalid_argument) {
                error_string = "Expected numerical 'layers' value in IrisRESTful GET tile command target URL.";
                goto MALFORMED_IRIS_REQUEST;
            }
            if (PARSE_FRONT_TOKEN(loc, end).compare("tiles")) {
                error_string = "Expected 'tiles' following layer index in IrisRESTful GET tile command target URL";
                goto MALFORMED_IRIS_REQUEST;
            }
            auto tile           = PARSE_FRONT_TOKEN(loc, end);
            result              = std::from_chars (tile.data(), tile.data()+tile.size(), request->tile);
            if (result.ec == std::errc::invalid_argument) {
                error_string = "Expected single numerical 'tiles' value in IrisRESTful GET tile command target URL.";
                goto MALFORMED_IRIS_REQUEST;
            }
            return request;
        }
        case GetRequest::GET_REQUEST_METADATA: {
            auto request = std::make_unique<GetMetadataRequest>();
            request->protocol   = GetRequest::GET_REQUEST_IRIS;
            request->type       = GetRequest::GET_REQUEST_METADATA;
            request->id         = PARSE_FRONT_TOKEN(loc, end);
            return request;
        }
    }
    
    error_string = "Undefined command sequence in IrisRESTful target URL. Please ensure your command conforms to the IrisRestful API.";
    
    MALFORMED_IRIS_REQUEST:
    auto request = std::make_unique<GetRequest>();
    request->protocol = GetRequest::GET_REQUEST_MALFORMED;
    request->error_msg = error_string;
    return request;
}

inline std::unique_ptr<GetRequest> PARSE_DICOM_REQUEST (const char* loc, const char* const end) {
    std::string error_string;
    
    if (PARSE_FRONT_TOKEN(loc, end).compare("studies"))
        throw std::runtime_error(std::string("PARSE_DICOM_REQUEST called on non-WADO-RS API GET request. Go to file ") +
                                 __FILE__ + " line " +std::to_string(__LINE__) + " to debug.");
    
    // Get the study identifier
    //    std::string_view study =
    PARSE_FRONT_TOKEN(loc, end); // STUDY Currently unused
    
    switch (PARSE_COMMAND(loc, end)) {
        case GetRequest::GET_REQUEST_UNDEFINED:
            error_string = "Undefined command sequence (last token) in DICOM/WADO-RS target URL. Please ensure your command conforms to IrisRestful API compliant WADO-RS commands.";
            goto MALFORMED_DICOM_REQUEST;
            
        case GetRequest::GET_REQUEST_TILE: {
            auto request = std::make_unique<GetTileRequest>();
            request->protocol   = GetRequest::GET_REQUEST_DICOM;
            request->type       = GetRequest::GET_REQUEST_TILE;
            if (PARSE_FRONT_TOKEN(loc, end).compare("series")) {
                error_string = "Expected 'series' following study identifier in DICOM/WADO-RS target URL.";
                goto MALFORMED_DICOM_REQUEST;
            }
            request->id         = PARSE_FRONT_TOKEN(loc, end);
            if (PARSE_FRONT_TOKEN(loc, end).compare("instances")) {
                error_string = "Expected 'instances' following series in DICOM/WADO-RS target URL.";
                goto MALFORMED_DICOM_REQUEST;
            }
            auto layer          = PARSE_FRONT_TOKEN(loc, end);
            auto result         = std::from_chars (layer.data(), layer.data()+layer.size(), request->layer);
            if (result.ec == std::errc::invalid_argument) {
                error_string = "Expected numerical 'instances' value in DICOM/WADO-RS target URL representing the resolution layer.";
                goto MALFORMED_DICOM_REQUEST;
            }
            if (PARSE_FRONT_TOKEN(loc, end).compare("frames")) {
                error_string = "Expected 'instances' following series in DICOM/WADO-RS target URL.";
                goto MALFORMED_DICOM_REQUEST;
            }
            auto tile           = PARSE_FRONT_TOKEN(loc, end);
            result              = std::from_chars (tile.data(), tile.data()+tile.size(), request->tile);
            if (result.ec == std::errc::invalid_argument) {
                error_string = "Expected numerical 'instances' value in DICOM/WADO-RS target URL representing the resolution layer.";
                goto MALFORMED_DICOM_REQUEST;
            }
            return request;
        }
        case GetRequest::GET_REQUEST_METADATA: {
            auto request = std::make_unique<GetMetadataRequest>();
            request->protocol   = GetRequest::GET_REQUEST_DICOM;
            request->type       = GetRequest::GET_REQUEST_METADATA;
            if (PARSE_FRONT_TOKEN(loc, end).compare("series")) {
                error_string = "Expected 'series' following study in DICOM/WADO-RS target URL. Please ensure metadata requests conform to IrisRestful API compliant WADO-RS commands.";
                goto MALFORMED_DICOM_REQUEST;
            }
            request->id         = PARSE_FRONT_TOKEN(loc, end);
            return request;
        }
    }
    
    // Any fall through
    error_string = "Undefined command sequence (last token) in DICOM/WADO-RS target URL. Please ensure your command conforms to IrisRestful API compliant WADO-RS commands.";
    
MALFORMED_DICOM_REQUEST:
    auto request = std::make_unique<GetRequest>();
    request->protocol = GetRequest::GET_REQUEST_MALFORMED;
    request->error_msg = error_string;
    return request;
}
inline std::unique_ptr<GetRequest> PARSE_FILE_REQUEST (const char* loc, const char* const end) {
    std::string error_string;
    auto test = std::string(loc);
    if (std::strcmp(loc, "/") == 0) {
        auto request        = std::make_unique<GetFileRequest>();
        request->protocol   = GetRequest::GET_REQUEST_FILE;
        request->path       = std::string("/index.html");
    } else if (loc[0] != '/' || std::string(loc).find("..") != std::string::npos) {
        error_string = "Illegal request-target";
        goto MALFORMED_FILE_REQUEST;
    } else {
        auto request        = std::make_unique<GetFileRequest>();
        request->protocol   = GetRequest::GET_REQUEST_FILE;
        request->path       = std::string(loc);
        if (!PARSE_MIME(loc, end, &request->mime)) {
            auto file_token = std::string(PARSE_BACK_TOKEN(loc, end));
            error_string = std::string("Unrecognized file type ") + file_token;
            goto MALFORMED_FILE_REQUEST;
        }
        return request;
    }
    
    
MALFORMED_FILE_REQUEST:
    auto request = std::make_unique<GetRequest>();
    request->protocol = GetRequest::GET_REQUEST_MALFORMED;
    request->error_msg = error_string;
    return request;
}
std::unique_ptr<Iris::RESTful::GetRequest>  parse_get_request (const std::string_view& target)
{
    // Make sure the request is lower-case to avoid non-match d/t case
    for (auto& c : target) const_cast<char&>(c) = tolower(c);
    const char* loc = &target.front();
    const char* const end = &target.back();
    
    // Parse the protocol. This will identify undefined protocols early and return
    // without having to parse the entire thing.
    switch (PARSE_PROTOCOL(loc, end)) {
        case GetRequest::GET_REQUEST_IRIS:
            return PARSE_IRIS_REQUEST(loc, end);
        case GetRequest::GET_REQUEST_DICOM:
            return PARSE_DICOM_REQUEST(loc, end);
        case GetRequest::GET_REQUEST_FILE:
            return PARSE_FILE_REQUEST(loc,end);
        default: {
            auto request = std::make_unique<GetRequest>();
            request->protocol = GetRequest::GET_REQUEST_MALFORMED;
            request->error_msg = "Undefined GET request protocol. Please follow either IrisRESTful or DICOMweb WADO-RS API";
            return request;
        }
    }
}
} // END IRIS
} // END 
