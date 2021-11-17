cmake_minimum_required(VERSION 3.10)
project(upsinfo)

set(CMAKE_CXX_STANDARD 11)

add_executable(upsinfo main.cpp)
target_link_libraries(upsinfo nutclient)