@echo off
chcp 65001 > nul

echo [1/3] Обновление репозитория...
git pull

if not exist build (
    mkdir build
)
cd build

echo [2/3] Конфигурация CMake...
cmake .. -G "MinGW Makefiles"

echo [3/3] Компиляция...
cmake --build .
