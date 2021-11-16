cmake_minimum_required(VERSION 3.14)
project(nutclient)

set(CMAKE_CXX_STANDARD 14)

set(BUILD_WITH_DEFAULT_SOCKET TRUE)


add_subdirectory(example)

if (BUILD_WITH_DEFAULT_SOCKET)
    add_compile_definitions(BUILD_WITH_DEFAULT_SOCKET)
endif(BUILD_WITH_DEFAULT_SOCKET)

add_library(nutclient SHARED nutclient.cpp)

if (BUILD_WITH_DEFAULT_SOCKET)
    target_sources(nutclient PRIVATE defaultsocket.h defaultsocket.cpp)
endif(BUILD_WITH_DEFAULT_SOCKET)

if (WIN32)
    add_compile_definitions(WIN32_EXPORT)
    if (BUILD_WITH_DEFAULT_SOCKET)
        target_link_libraries(nutclient Ws2_32.dll)
    endif(BUILD_WITH_DEFAULT_SOCKET)
endif(WIN32)