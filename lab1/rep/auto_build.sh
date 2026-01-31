# 1. Обновление с Git
echo "[1/3] Обновление репозитория..."
git pull

# 2. Создание папки сборки
if [ ! -d "build" ]; then
    echo "Создаем папку build..."
    mkdir build
fi

cd build

# 3. Конфигурация CMake
echo "[2/3] Конфигурация CMake..."
cmake ..

# 4. Компиляция
echo "[3/3] Компиляция..."
make
