cmake_minimum_required(VERSION 3.10)
project(main CXX)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)

find_package(Threads REQUIRED)

add_executable(Main src/main.cpp src/main_utils.cpp)

if(UNIX)
    target_link_libraries(Main Threads::Threads rt)
else()
    target_link_libraries(Main Threads::Threads)
endif()