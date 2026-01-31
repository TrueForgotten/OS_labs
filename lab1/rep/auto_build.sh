echo "[1/3] Обновление репозитория..."
git pull

if [ ! -d "build" ]; then
    mkdir build
fi

cd build

echo "[2/3] Конфигурация CMake..."
cmake ..

echo "[3/3] Компиляция..."
make
