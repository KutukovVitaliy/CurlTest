cmake_minimum_required(VERSION 3.20)
project(CurlTest C)

set(CMAKE_C_STANDARD 99)

add_executable(CurlTest main.c)
target_link_libraries(CurlTest curl)