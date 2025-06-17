/**
 * @file IrisRestfulSlide.hpp
 * @author Ryan Landvater (ryanlandvater [at] gmail [dot] com)
 * @brief 
 * @version 0.1
 * @date 2025-06-07
 * 
 * @copyright Copyright (c) 2025 Iris Developers
 * 
 */

#ifndef IrisRestfulSlide_hpp
#define IrisRestfulSlide_hpp
namespace Iris {
namespace RESTful {
Slide validate_and_open_slide (const std::filesystem::path& file_path);
class __INTERNAL__Slide {
    friend class __INTERNAL__Server;
    const std::string                   _id;
    const IrisCodec::File               _file;
    const IrisCodec::Abstraction::File  _abstraction;
    std::function<void()>               _remove_from_server_dir;
protected:
    void  set_on_destroyed_callback     (const std::function<void()>);
public:
    explicit __INTERNAL__Slide          (const IrisCodec::File&);
    __INTERNAL__Slide                   (const __INTERNAL__Server&) = delete;
    __INTERNAL__Slide& operator ==      (const __INTERNAL__Server&) = delete;
   ~__INTERNAL__Slide                   ();
    
    bool operator !=                    (std::string&) const;
    SlideInfo           get_slide_info  () const;
    Buffer              get_tile_entry  (uint32_t layer, uint32_t tile_indx) const;
};
}
}
#endif /* IrisRestfulSlide_hpp */
