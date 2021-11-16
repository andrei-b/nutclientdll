cmake_minimum_required(VERSION 3.14)
project(nutclient)

set(CMAKE_CXX_STANDARD 14)

set(BUILD_WITH_DEFAULT_SOCKET TRUE)
set(NUTCLIENT_DYNAMIC_LIB FALSE)

add_subdirectory(example)

if (BUILD_WITH_DEFAULT_SOCKET)
    add_compile_definitions(BUILD_WITH_DEFAULT_SOCKET)
endif(BUILD_WITH_DEFAULT_SOCKET)

if (NUTCLIENT_DYNAMIC_LIB)
    set(LIB_TYPE SHARED)
    if (WIN32)
        add_compile_definitions(WIN32_EXPORT)
    endif(WIN32)
else(NUTCLIENT_DYNAMIC_LIB)
    set(LIB_TYPE STATIC)
    add_compile_definitions(BUILD_STATIC)
endif(NUTCLIENT_DYNAMIC_LIB)

if (BUILD_WITH_DEFAULT_SOCKET)
    set(SOURCES "nutclient.cpp" "defaultsocket.cpp" "defaultsocket.h")
else(BUILD_WITH_DEFAULT_SOCKET)
    set(SOURCES "nutclient.cpp")
endif(BUILD_WITH_DEFAULT_SOCKET)

add_library(nutclient ${LIB_TYPE} ${SOURCES})

if (WIN32)
    if (BUILD_WITH_DEFAULT_SOCKET)
        target_link_libraries(nutclient Ws2_32.dll)
    endif(BUILD_WITH_DEFAULT_SOCKET)
endif(WIN32)