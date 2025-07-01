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
 * @param info Create info structure detailing the directory containing the Iris slide files
 * and the directory containing the SSL certs and keys for the secure socket layer (HTTPS)
 * @return Server
 */
Server create_server (const ServerCreateInfo& info);
/**
 * @brief Instruct the server to begin listening at the provided port
 * 
 * @param port port number to begin listening on
 * @return Result flag indicating success or failure
 */
Result server_listen (const Server&, uint16_t port);
} // END RESTFUL NAMESPACE
} // END IRIS NAMESPACE
