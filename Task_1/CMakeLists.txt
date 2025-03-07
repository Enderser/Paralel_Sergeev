cmake_minimum_required(VERSION 3.10)
project(SinArraySum)

option(USE_DOUBLE "Use double instead of float" ON)

if(USE_DOUBLE)
    add_compile_definitions(USE_DOUBLE=1)
else()
    add_compile_definitions(USE_DOUBLE=0)
endif()

add_executable(sin_array_sum main.cpp)