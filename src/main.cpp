//
//  main.cpp
//  IrisRESTful
//
//  Created by Ryan Landvater on 6/7/25.
//
#include <csignal>
#include <charconv>
#include <iostream>
#include "IrisResfultCore.hpp"

constexpr char intro_statement[] =
"Iris RESTful Server is a high-performance HTTPS server implementation \
that provides access to slide data within Iris Codec file extension format (.iris)\n\
For more information visit the official repo at https://github.com/IrisDigitalPathology/Iris-RESTful-Server.git\n\
The Iris RESTful server is Licensed under the MIT Sofware License and is \
Copyright (c) 2025 Ryan Landvater \n";

constexpr char help_statement[] =
"Arugments:\n\
-h --help: Print this help text \n\
-p --port: \n\
-d --dir: Directory path to the directory containing the Iris Slide Files to be served\n\
-c --cert: Public SSL certificate in PEM format for establishing HTTPS connections\n\
-k --key: Private key in PEM format to sign argument provided in CERT\n\
-o --cors: Slide viewer domain. Returned in 'Access-Control-Allow-Origin' header\n\
-r --root: Web viewer server document root directory for activating RESTful server as file server\n\
If run without defining the -r/--root option, HTTPS responses will contain \
'Access-Control-Allow-Origin':'*' unless the `-o/--cors option` is defined. \n\
\n\
Usage: IrisRESTful -p <port> -d <slide_root> -c <cert.pem> -k <key.pem> -r <document_root>\n\
Example:\n\tIrisRESTful -p 3000 -d /slides -c /ect/ssl/iris_cert.pem -k /ect/ssl/private/iris_key.pem -r /openseadragon\n\
\n";

enum ArgumentFlag : uint32_t {
    ARG_HELP    = 0,
    ARG_PORT,
    ARG_DIR,
    ARG_CERT,
    ARG_KEY,
    ARG_CORS,
    ARG_ROOT,
    ARG_INVALID = UINT32_MAX
};

inline ArgumentFlag PARSE_ARGUMENT (const char* arg_str) {
    if (strstr(arg_str,"-h") || strstr(arg_str, "--help"))
        return ARG_HELP;
    if (strstr(arg_str,"-p") || strstr(arg_str, "--port"))
        return ARG_PORT;
    if (strstr(arg_str,"-d") || strstr(arg_str, "--dir"))
        return ARG_DIR;
    if (strstr(arg_str,"-c") || strstr(arg_str, "--cert"))
        return ARG_CERT;
    if (strstr(arg_str,"-k") || strstr(arg_str, "--key"))
        return ARG_KEY;
    if (strstr(arg_str,"-o") || strstr(arg_str, "--cors"))
        return ARG_CORS;
    if (strstr(arg_str,"-r") || strstr(arg_str, "--root"))
        return ARG_ROOT;
    return ARG_INVALID;
}

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
    int port = -1;
    Iris::RESTful::ServerCreateInfo info;
    if (argc < 2) {
        std::cerr << "Insufficient arguments\n" << intro_statement << help_statement;
        return EXIT_FAILURE;
    } else for (auto argi = 1; argi < argc; ++argi) {
        switch (PARSE_ARGUMENT(argv[argi])) {
            case ARG_HELP:
                std::cout << intro_statement << help_statement;
                return EXIT_SUCCESS;
            case ARG_PORT: {
                std::string port_str (argv[++argi]);
                auto result = std::from_chars(port_str.data(), port_str.data()+port_str.size(), port);
                if (result.ec != std::errc{}) {
                    std::cerr   << "Failed to parse the provided port number from argument \""
                                << port_str << "\"\n"
                                << help_statement;
                    return EXIT_FAILURE;
                }
                if (port > UINT16_MAX) {
                    std::cerr   << "Provided port number outside of the the valid port range (0-65535) \""
                                << port_str << "\"\n"
                                << help_statement;
                }
                break;
            }
                
            case ARG_DIR:
                if (argi+1>=argc) {
                    FAILED_SLIDE_DIRECTORY:
                    std::cerr   << "Slide directory argument requires directory path\n"
                                << help_statement;
                    return EXIT_FAILURE;
                }
                info.slide_dir = std::string(argv[++argi]);
                if (std::filesystem::is_directory(info.slide_dir) == false) {
                    std::cerr   << "OS reports the provided path for slide files \""
                                << info.slide_dir
                                << "\" is an invalide directory path.\n"
                                << help_statement;
                    return EXIT_FAILURE;
                }
                break;
                
            case ARG_CERT:
                if (argi+1>=argc) {
                    std::cerr   <<"Certificate argument requires PEM formatted cert file path\n"
                                << help_statement;
                    return EXIT_FAILURE;
                }
                info.cert = std::string(argv[++argi]);
                if (std::filesystem::exists(info.cert) == false) {
                    std::cerr   << "OS reports the provided file path for server cert \""
                                << info.cert
                                << "\" is an invalide file path.\n\n"
                                << help_statement;
                    return EXIT_FAILURE;
                }
                break;
                
            case ARG_KEY:
                if (argi+1>=argc) {
                    std::cerr   <<"Private key argument requires PEM formatted key file path\n"
                                << help_statement;
                    return EXIT_FAILURE;
                }
                info.key = std::string(argv[++argi]);
                if (std::filesystem::exists(info.key) == false) {
                    std::cerr   << "OS reports the provided file path for server private key "
                                << info.key
                                << " is an invalide PEM key file path.\n\n"
                                << help_statement;
                    return EXIT_FAILURE;
                }
                break;
                
            case ARG_CORS:
                if (argi+1>=argc) {
                    std::cerr   <<"Cross origin resource sharing requires a valid domain\n"
                                << help_statement;
                    return EXIT_FAILURE;
                }
                info.cors = std::string(argv[++argi]);
                break;
                
            case ARG_ROOT:
                if (argi+1>=argc) {
                    FAILED_ROOT_FILE_DIR:
                    std::cerr   <<"Root file directory requires a valid file directory\n"
                                << help_statement;
                    return EXIT_FAILURE;
                }
                info.doc_root = std::string(argv[++argi]);
                if (std::filesystem::is_directory(info.doc_root) == false) {
                    std::cerr   << "OS reports the provided document root file path \""
                                << info.doc_root
                                << "\" is an invalide directory path.\n"
                                << help_statement;
                    return EXIT_FAILURE;
                }
                break;
                
            case ARG_INVALID:
                std::cerr   << "Unknown argument \""
                            << argv[argi]
                            << "\"\n"
                            << help_statement;
                return EXIT_FAILURE;
        }
    }
    
    auto server = Iris::RESTful::create_server (info);
    if (!server) {
        std::cout << "Failed to create an Iris RESTful server\n";
        return EXIT_FAILURE;
    }
    
    auto result = Iris::RESTful::server_listen(server, port>-1?port:3000);
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
