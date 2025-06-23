# 2025 Copyright Ryan Landvater
# Created 3/3/2025

cmake_minimum_required(VERSION 3.27)
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_CXX_FLAGS "-O3") # Agressively optimize for speed
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_SERVER "Build the RESTful Server Implementation" ON)
option(IRIS_BUILD_SHARED "Build IrisCodec Shared Library" ON)
option(IRIS_BUILD_STATIC "Build IrisCodec Static Library" ON) 

PROJECT (
    IrisRESTfulServer
    LANGUAGES CXX
    # VERSION ${IRIS_CODEC_VERSION_STRING}
)

# Include the Boost libraries for ASIO support
find_package(Boost REQUIRED CONFIG)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Iris Core Headers Inclusion
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
FetchContent_Declare (
    IrisHeaders
    GIT_REPOSITORY https://github.com/IrisDigitalPathology/Iris-Headers.git
    GIT_TAG "origin/main"
    GIT_SHALLOW ON
    FETCHCONTENT_UPDATES_DISCONNECTED ON
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Iris File Extension Inclusion
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Do not export the IFE API. 
set(IFE_BUILD_SHARED OFF)
set(IFE_BUILD_STATIC OFF)
# NOTE: The following IFE_Export definition
# is IFE_EXPORT= to blank out import/export
# declarations. This is important as we use
# intermediate objects rather than link to
# the Iris File Extension Library (MSVC)
add_compile_definitions(IFE_EXPORT=)
FetchContent_Declare (
    IrisFileExtension
    GIT_REPOSITORY https://github.com/IrisDigitalPathology/Iris-File-Extension.git
    GIT_TAG "origin/main"
    GIT_SHALLOW ON
    FETCHCONTENT_UPDATES_DISCONNECTED ON
    FETCHCONTENT_QUIET ON
)
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Iris Codec Inclusion (Only some components)
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
FetchContent_Declare (
    IrisCodec
    GIT_REPOSITORY https://github.com/IrisDigitalPathology/Iris-Codec.git
    GIT_TAG "origin/main"
    GIT_SHALLOW ON
    FETCHCONTENT_UPDATES_DISCONNECTED ON
    FETCHCONTENT_QUIET ON
    SOURCE_SUBDIR do_not_build
)
message(STATUS "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~")
FetchContent_MakeAvailable(
    IrisHeaders
    IrisFileExtension
    IrisCodec
)
message(STATUS "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~")
# Threading
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Iris RESTful Server Universal Object Build
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
set(BOOST_SOURCE_DIR ${boost_SOURCE_DIR}/libs)
set(IFE_SOURCE_DIR ${irisfileextension_SOURCE_DIR}/src)
set(CODEC_SOURCE_DIR ${iriscodec_SOURCE_DIR}/src)
set(SERVER_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(SERVER_PRIV_DIR ${PROJECT_SOURCE_DIR}/priv)
set(SERVER_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set (
    ServerSources
    ${CODEC_SOURCE_DIR}/IrisCodecFile.cpp
    ${SERVER_PRIV_DIR}/IrisAsync.cpp
    ${SERVER_SOURCE_DIR}/IrisRestfulGetParser.cpp
    ${SERVER_SOURCE_DIR}/IrisRestfulGetSerializer.cpp
    ${SERVER_SOURCE_DIR}/IrisRestfulNetworking.cpp
    ${SERVER_SOURCE_DIR}/IrisRestfulServer.cpp
    ${SERVER_SOURCE_DIR}/IrisRestfulSlide.cpp
)
set (
    ServerInclude
    ${SERVER_INCLUDE_DIR}
    ${SERVER_PRIV_DIR}
    ${SERVER_SOURCE_DIR}
    ${IFE_SOURCE_DIR}
    ${CODEC_SOURCE_DIR}
    ${irisheaders_SOURCE_DIR}/include
    ${irisheaders_SOURCE_DIR}/priv
    ${Boost_INCLUDE_DIRS}
)
add_library (
    IrisRestfulLib OBJECT
    ${ServerSources}
)
target_include_directories (
    IrisRestfulLib PRIVATE
    ${ServerInclude}
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Iris RESTful Server Targets
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
set (
    IrisRESTfulTargets
    IrisRestfulLib
)

if (BUILD_SERVER)
    add_executable(
        IrisRESTful
        ${SERVER_SOURCE_DIR}/main.cpp
        $<TARGET_OBJECTS:IrisFileExtensionLib>
        $<TARGET_OBJECTS:IrisRestfulLib>
    )
    target_link_libraries(
        IrisRESTful
        PRIVATE Threads::Threads
    )
    target_include_directories (
        IrisRESTful PRIVATE
        ${ServerInclude}
    )
    set(IrisRESTfulTargets ${IrisRESTfulTargets} IrisRESTful)
endif()

if (IRIS_BUILD_SHARED)
    add_library(
        IrisRestfulShared SHARED 
        $<TARGET_OBJECTS:IrisFileExtensionLib>
        $<TARGET_OBJECTS:IrisRestfulLib>
    )
    target_link_libraries(
        IrisRestfulShared
        PRIVATE Threads::Threads
    )
    set(IrisRESTfulTargets ${IrisRESTfulTargets} IrisRestfulShared)
    set_target_properties(IrisRestfulShared PROPERTIES OUTPUT_NAME "IrisRestful")
endif()

if (IRIS_BUILD_STATIC)
    add_library(
        IrisRestfulStatic STATIC 
        $<TARGET_OBJECTS:IrisFileExtensionLib>
        $<TARGET_OBJECTS:IrisRestfulLib>
    )
    target_link_libraries(
        IrisRestfulStatic
        PRIVATE Threads::Threads
    )
    set(IrisRESTfulTargets ${IrisRESTfulTargets} IrisRestfulStatic)
endif(IRIS_BUILD_STATIC)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Installation
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

if (${PROJECT_IS_TOP_LEVEL})

    install(
        TARGETS IrisHeaders
        EXPORT IrisHeadersConfig
        FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Iris
    )
    install(
        EXPORT IrisHeadersConfig
        FILE IrisHeadersConfig.cmake
        NAMESPACE "Iris::"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Iris
    )
    install(
        TARGETS ${IrisRESTfulTargets}
        EXPORT IrisRESTfulConfig
        FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Iris
    )
    install(
        EXPORT IrisRESTfulConfig
        FILE IrisRESTfulConfig.cmake
        NAMESPACE "Iris::"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Iris
    )
endif()
