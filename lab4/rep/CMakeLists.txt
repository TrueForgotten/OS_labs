cmake_minimum_required(VERSION 3.10)
project(SerialPortLab)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

# Эмулятор 
add_executable(emulator emulator.cpp my_serial.hpp)

# Логгер 
add_executable(logger logger.cpp my_serial.hpp)

if(UNIX)
    target_link_libraries(emulator PRIVATE Threads::Threads rt)
    target_link_libraries(logger PRIVATE Threads::Threads rt)
else()
    target_link_libraries(emulator PRIVATE Threads::Threads)
    target_link_libraries(logger PRIVATE Threads::Threads)
endif()