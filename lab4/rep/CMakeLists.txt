cmake_minimum_required(VERSION 3.10)
project(SerialPortLab)

# Указываем стандарт C++17 (нужен для std::filesystem и chrono)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Находим библиотеку потоков (нужна для std::thread и sleep_for)
find_package(Threads REQUIRED)

# --- Цель 1: Эмулятор ---
# Создаем исполняемый файл "emulator" из исходников emulator.cpp и хедера
add_executable(emulator emulator.cpp my_serial.hpp)

# --- Цель 2: Логгер ---
# Создаем исполняемый файл "logger" из исходников logger.cpp и хедера
add_executable(logger logger.cpp my_serial.hpp)

# Линковка библиотек с учетом ОС
if(UNIX)
    # На Linux/Unix часто нужна библиотека rt для работы с таймерами (std::chrono)
    target_link_libraries(emulator PRIVATE Threads::Threads rt)
    target_link_libraries(logger PRIVATE Threads::Threads rt)
else()
    # На Windows rt не нужна
    target_link_libraries(emulator PRIVATE Threads::Threads)
    target_link_libraries(logger PRIVATE Threads::Threads)
endif()