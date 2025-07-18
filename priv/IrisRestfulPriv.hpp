/**
 * @file IrisRestfulPriv.hpp
 * @author Ryan Landvater (ryanlandvater [at] gmail [dot] com)
 * @brief 
 * @version 0.1
 * @date 2025-06-07
 * 
 * @copyright Copyright (c) 2025 Iris Developers
 * 
 */

#include <assert.h>
#include <iostream>
#include "IrisRestfulTypes.hpp"
#include "IrisQueue.hpp"
#include "IrisAsync.hpp"
#include "IrisCodecPriv.hpp"
#include "IrisRestfulNetworking.hpp"
#include "IrisRestfulSlide.hpp"
#include "IrisRestfulServer.hpp"
#include "IrisResfultCore.hpp"
namespace Iris {
namespace RESTful {
std::unique_ptr<GetRequest>  parse_get_request   (const std::string_view& target);
std::unique_ptr<PostRequest> parse_post_request  (const std::string_view& target); // Not defined yet
std::unique_ptr<PutRequest>  parse_put_request   (const std::string_view& target); // Not defined yet
// TODO: Consider just creating a JSON serializer
std::string serialize_get_response (const GetResponse& response);

}
}
