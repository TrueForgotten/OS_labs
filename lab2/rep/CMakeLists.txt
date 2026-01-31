cmake_minimum_required(VERSION 3.10)
project(lab2_background_launcher)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)

find_package(Threads REQUIRED)

add_library(launcher_lib STATIC src/background_launcher.cpp)
add_executable(Main src/main.cpp)

target_link_libraries(Main launcher_lib Threads::Threads)