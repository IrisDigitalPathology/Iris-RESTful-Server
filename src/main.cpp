//
//  main.cpp
//  IrisRESTful
//
//  Created by Ryan Landvater on 6/7/25.
//
#include <iostream>
#include <csignal>
#include "IrisResfultCore.hpp"
volatile sig_atomic_t terminate_flag = 0;
void INTERP_CSIGNAL (int param)
{
    switch (param) {
        case SIGINT:
        case SIGTERM:
        case SIGQUIT:
            std::cout << "Shutting down..." << std::endl;
            terminate_flag = 1;
            return;
        default:
            break;
    }
}
int main(int argc, char* argv[])
{
    // Check command line arguments.
    if (argc != 3)
    {
        std::cerr   << "Usage: IrisRESTful <port> <slide_root>\n"
                    << "Example:\n\tIrisRESTful 3000 /slide_directory \n";
        return EXIT_FAILURE;
    }
    std::string port_string = std::string(argv[1]);
    unsigned int port;
    auto success = std::from_chars (port_string.data(), port_string.data()+port_string.size(), port);
    if (success.ec == std::errc::invalid_argument || port > UINT16_MAX) {
        std::cerr   << "Invalid port given as arguemt 1.\n"
                    << "Usage: IrisRESTful <port> <slide_root>\n"
                    << "Example:\n\tIrisRESTful 3000 /slide_directory \n";
        return EXIT_FAILURE;
    }
    
    std::filesystem::path slide_dir_path(argv[2]);
    if (std::filesystem::is_directory(slide_dir_path) == false) {
        std::cerr   << "Invalid port given as arguemt 1.\n"
                    << "Usage: IrisRESTful <port> <slide_root>\n"
                    << "Example:\n\tIrisRESTful 3000 /slide_directory \n";
    }
    auto server = Iris::RESTful::create_server(slide_dir_path);
    if (!server) {
        std::cout << "Failed to create an Iris RESTful server\n";
        return EXIT_FAILURE;
    }
    
    auto result = Iris::RESTful::server_listen(server, 3000);
    if (result != Iris::IRIS_SUCCESS) {
        std::cout << result.message;
        return EXIT_FAILURE;
    }
    
    signal(SIGTERM,INTERP_CSIGNAL);
    signal(SIGINT,INTERP_CSIGNAL);
    signal(SIGQUIT,INTERP_CSIGNAL);
    
    while (!terminate_flag)
        std::this_thread::sleep_for(std::chrono::seconds(1));
    
    return EXIT_SUCCESS;
}
