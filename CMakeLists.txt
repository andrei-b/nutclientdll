cmake_minimum_required(VERSION 3.16)
project(nutclient)

set(CMAKE_CXX_STANDARD 17)

add_library(nutclient SHARED nutclient.cpp simplesocket.h)

if (WIN32)
    add_compile_definitions(WIN32_EXPORT)
    target_link_libraries(nutclient Ws2_32.dll)
endif(WIN32)