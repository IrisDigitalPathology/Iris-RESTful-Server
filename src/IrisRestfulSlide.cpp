/**
 * @file IrisRestfulSlide.cpp
 * @author Ryan Landvater (ryanlandvater [at] gmail [dot] com)
 * @brief 
 * @version 0.1
 * @date 2025-06-07
 * 
 * @copyright Copyright (c) 2025 Iris Developers
 * 
 */

#include "IrisRestfulPriv.hpp"

Iris::RESTful::Slide Iris::RESTful::validate_and_open_slide (const std::filesystem::path &file_path)
{
    using namespace IrisCodec;
    
    if (!std::filesystem::exists(file_path)) throw std::runtime_error
        ("File does not exist");
    
    auto file = open_file(FileOpenInfo {
        .filePath = file_path,
        .writeAccess = false,
    });
    auto ptr    = file->ptr;
    auto size   = file->size;
    
    // Test that the file is an Iris codec file
    if (!IrisCodec::is_Iris_Codec_file(ptr, size)) throw std::runtime_error
        ("Not an Iris slide file");
    
    // Validate the file structure
    auto result = IrisCodec::validate_file_structure(ptr, size);
    if (result & IRIS_FAILURE) throw std::runtime_error
        ("File failed validation: " + result.message);
    
    // Return the new Iris File
    return std::make_shared<__INTERNAL__Slide>(file);
}

namespace Iris {
namespace RESTful {
using namespace IrisCodec;
__INTERNAL__Slide::__INTERNAL__Slide(const File &file) :
_file                   (file),
_abstraction            (abstract_file_structure(file->ptr, file->size)),
_remove_from_server_dir (nullptr)
{
    std::cout << "SLIDE CREATED\n";
}
__INTERNAL__Slide::~__INTERNAL__Slide()
{
    std::cout << "SLIDE REMOVED\n";
    if (_remove_from_server_dir) _remove_from_server_dir ();
}
void __INTERNAL__Slide::set_on_destroyed_callback(const std::function<void()> on_destroyed)
{
    _remove_from_server_dir = on_destroyed;
}
bool __INTERNAL__Slide::operator!=(std::string &id) const
{
    return _id.compare(id);
}
SlideInfo __INTERNAL__Slide::get_slide_info() const
{
    return SlideInfo {
        .format         = _abstraction.tileTable.format,
        .encoding       = _abstraction.tileTable.encoding,
        .extent         = _abstraction.tileTable.extent,
        .metadata       = _abstraction.metadata,
    };
}
Buffer __INTERNAL__Slide::get_tile_entry (uint32_t layer, uint32_t tile_indx) const
{
    ReadLock lock (_file->resize);
    
    // Pull the extent and check that the layer in within info
    auto& ttable = _abstraction.tileTable;
    auto& layers = ttable.layers;
    if (layer >= layers.size()) throw std::runtime_error
        ("layer in SlideTileReadInfo is out of bounds");
    
    // Pull the layer extent and check that the tile is within info
    auto& tiles = layers[layer];
    if (tile_indx >= tiles.size()) throw std::runtime_error
        ("tile in SLideTileReadInfo is out of layer bounds");
    
    // Get the offset and size of the tile entry
    auto& entry = tiles[tile_indx];
    return Iris::Copy_strong_buffer_from_data(_file->ptr + entry.offset, entry.size);
}
} // END RESTFUL
} // END IRIS
