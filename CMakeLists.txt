cmake_minimum_required(VERSION 3.10)
project(IoT_Server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_library(sqlite_lib sqlite/sqlite3.c)

# --- Эмулятор ---
add_executable(emulator emulator.cpp my_serial.hpp)
target_link_libraries(emulator PRIVATE Threads::Threads)

# --- Логгер (Server) ---
add_executable(logger logger.cpp my_serial.hpp db_manager.hpp)

# Линковка для Windows (httplib нужны ws2_32 и crypt32)
if(WIN32)
    target_link_libraries(logger PRIVATE sqlite_lib ws2_32 crypt32 Threads::Threads)
else()
    target_link_libraries(logger PRIVATE sqlite_lib Threads::Threads dl rt)
endif()