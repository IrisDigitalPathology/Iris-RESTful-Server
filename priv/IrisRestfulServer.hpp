/**
 * @file IrisRestfulServer.hpp
 * @author Ryan Landvater (ryanlandvater [at] gmail [dot] com)
 * @brief 
 * @version 0.1
 * @date 2025-06-07
 * 
 * @copyright Copyright (c) 2025 Iris Developers
 * 
 */

#ifndef IrisRestfulServer_hpp
#define IrisRestfulServer_hpp
namespace Iris {
namespace RESTful {

class __INTERNAL__Server {
    friend class __INTERNAL__Networking;
    const std::filesystem::path     _root;
    const std::filesystem::path     _doc_root;
    struct : public std::unordered_map<std::string,
    std::weak_ptr<__INTERNAL__Slide>> {
        SharedMutex                 mutex;
    }                               _directory;
    Networking                      _networking;
    Async::ThreadPool               _threads;
public:
    explicit __INTERNAL__Server     (const ServerCreateInfo&);
    __INTERNAL__Server              (const __INTERNAL__Server&) = delete;
    __INTERNAL__Server& operator == (const __INTERNAL__Server&) = delete;
    void listen                     (uint16_t port);
    
protected:
    template <class Session_>
    void    on_get_request          (const Session_&,
                                     const std::string&,
                                     const std::function<void(const std::unique_ptr<GetResponse>&)>);
    
private:
    Slide   get_slide               (const std::string& idenfifier);
    
//    void on_post_request            (const Session&,
//                                     const std::string_view&,
//                                     const std::function<void(std::unique_ptr<PostResponse&>)>);
//    void on_put_request             (const Session&,
//                                     const std::string_view&,
//                                     const std::function<void(std::unique_ptr<PutRequest&>)>);
    
};
} // END NAMESPACE RESTful
} // END NAMESPACE IRIS
#endif /* IrisRestfulServer_hpp */
