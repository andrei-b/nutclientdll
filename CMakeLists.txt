cmake_minimum_required(VERSION 3.10)
project(nutclient)

set(CMAKE_CXX_STANDARD 11)

if(NOT DEFINED NUTCLIENT_DYNAMIC_LIB)
    set(NUTCLIENT_DYNAMIC_LIB TRUE)
endif(NOT DEFINED NUTCLIENT_DYNAMIC_LIB)

if(NOT DEFINED NUTCLIENT_BUILD_WITH_DEFAULT_SOCKET)
    set(NUTCLIENT_BUILD_WITH_DEFAULT_SOCKET TRUE)
endif(NOT DEFINED NUTCLIENT_BUILD_WITH_DEFAULT_SOCKET)

add_subdirectory(example)

if (NUTCLIENT_BUILD_WITH_DEFAULT_SOCKET)
    add_compile_definitions(BUILD_WITH_DEFAULT_SOCKET)
endif(NUTCLIENT_BUILD_WITH_DEFAULT_SOCKET)

if (NUTCLIENT_DYNAMIC_LIB)
    set(LIB_TYPE SHARED)
    if (WIN32)
        add_compile_definitions(WIN32_EXPORT)
    endif(WIN32)
else(NUTCLIENT_DYNAMIC_LIB)
    set(LIB_TYPE STATIC)
    add_compile_definitions(BUILD_STATIC)
endif(NUTCLIENT_DYNAMIC_LIB)

if (NUTCLIENT_BUILD_WITH_DEFAULT_SOCKET)
    set(SOURCES "nutclient.cpp" "nutclient.h" "defaultsocket.cpp" "defaultsocket.h")
else(NUTCLIENT_BUILD_WITH_DEFAULT_SOCKET)
    set(SOURCES "nutclient.cpp" "nutclient.h")
endif(NUTCLIENT_BUILD_WITH_DEFAULT_SOCKET)

add_library(nutclient ${LIB_TYPE} ${SOURCES})

if (WIN32)
    if (NUTCLIENT_BUILD_WITH_DEFAULT_SOCKET)
        target_link_libraries(nutclient Ws2_32.dll)
    endif(NUTCLIENT_BUILD_WITH_DEFAULT_SOCKET)
endif(WIN32)