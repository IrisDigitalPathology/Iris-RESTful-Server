/**
 * @file IrisResfultCore.hpp
 * @author Ryan Landvater (ryanlandvater [at] gmail [dot] com)
 * @brief A light implementation for OpenSeaDragon based viewer implementations
 * that allows access to the Iris slide format. 
 * @version 0.1
 * @date 2025-06-07
 * 
 * @copyright Copyright (c) 2025 Iris Developers
 * 
 */

#include "IrisRestfulTypes.hpp"

namespace Iris {
namespace RESTful {
/**
 * @brief Create a server object
 * 
 * @param root_slide_dir 
 * @return Server 
 */
Server create_server (const std::filesystem::path& root_slide_dir);
/**
 * @brief Instruct the server to begin listening at the provided port
 * 
 * @param port 
 * @return Result flag indicating success or failure
 */
Result server_listen (const Server&, uint16_t port);
} // END RESTFUL NAMESPACE
} // END IRIS NAMESPACE
