/**
 * @file IrisRestfulGetSerializer.cpp
 * @author Ryan Landvater (ryanlandvater [at] gmail [dot] com)
 * @brief 
 * @version 0.1
 * @date 2025-06-07
 * 
 * @copyright Copyright (c) 2025 Iris Developers
 * 
 */

#include <streambuf>
#include "IrisRestfulPriv.hpp"

namespace Iris {
namespace RESTful {
inline std::string SERIALIZE_FORMAT (const Iris::Format &format)
{
    switch (format) {
        case FORMAT_UNDEFINED:  return "\"FORMAT_UNDEFINED\"";
        case FORMAT_B8G8R8:     return "\"FORMAT_B8G8R8\"";
        case FORMAT_R8G8B8:     return "\"FORMAT_R8G8B8\"";
        case FORMAT_B8G8R8A8:   return "\"FORMAT_B8G8R8A8\"";
        case FORMAT_R8G8B8A8:   return "\"FORMAT_R8G8B8A8\"";
    }
}
inline std::string SERIALIZE_ENCODING (const IrisCodec::Encoding &encoding)
{
    switch (encoding) {
        case IrisCodec::TILE_ENCODING_UNDEFINED:    return "\"ENCODING_UNDEFINED\"";
        case IrisCodec::TILE_ENCODING_IRIS:         return "\"image/iris\"";
        case IrisCodec::TILE_ENCODING_JPEG:         return "\"image/jpeg\"";
        case IrisCodec::TILE_ENCODING_AVIF:         return "\"image/avif\"";
    }
}
inline void SERIALIZE_LAYER_EXTENT (const LayerExtents &extent, std::stringstream& stream)
{
    stream << "[";
    for (auto&& layer : extent) {
        stream  << "{"
                << "\"x_tiles\": " << layer.xTiles << ","
                << "\"y_tiles\": " << layer.yTiles << ","
                << "\"scale\": " << layer.scale
                << "},";
    }
    stream.seekp(-1,stream.cur) << "]";
}
inline void SERALIZE_SLIDE_EXTENT (const Extent &extent, std::stringstream& stream)
{
    stream  << "{"
            << "\"width\": " << extent.width << ","
            << "\"height\": " << extent.height << ","
            << "\"layers\": ";
    
    SERIALIZE_LAYER_EXTENT(extent.layers, stream);
    stream << "},";
}
inline std::string SERIALIZE_SLIDE_METADATA_JSON (const GetResponse& response)
{
    assert(response.type == GetResponse::GET_RESPONSE_METADATA && "");
    auto& info = reinterpret_cast<const GetMetadataResponse&>(response).slideInfo;
    
    std::stringstream stream;
    stream << "{";
    stream << "\"type\": \"slide_metadata\",";
    if (info.format)
        stream  <<"\"format\": " << SERIALIZE_FORMAT (info.format) << ",";
    if (info.encoding)
        stream  <<"\"encoding\": " << SERIALIZE_ENCODING(info.encoding) << ",";
    
    stream <<"\"extent\": ";
    SERALIZE_SLIDE_EXTENT(info.extent,stream);
    
    stream.seekp(-1,stream.cur) << "}";
    return stream.str();
}
std::string serialize_get_response (const GetResponse& response)
{
    switch (response.type) {
        case GetResponse::GET_RESPONSE_UNDEFINED:
        case GetResponse::GET_RESPONSE_MALFORMED_REQ:
        case GetResponse::GET_RESPONSE_FILE_NOT_FOUND:
            return response.error_msg.size()?response.error_msg:
            "Undefined GET request error. IrisRESTful server did elaborate on what happened.";
        case GetResponse::GET_RESPONSE_METADATA:
            return SERIALIZE_SLIDE_METADATA_JSON(response);
        case GetResponse::GET_RESPONSE_TILE:
            assert(false && "ERROR: cannot perform serialize_get_response on GET_RESPONSE_TILE response; this is a binary response");
            throw std::runtime_error("ERROR: cannot serialize_get_response a GET_RESPONSE_TILE response; this is a binary response");
    }
}
} // END RESTFUL
} // END IRIS
