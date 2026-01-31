@echo off
:: Переходим в папку скрипта
cd /d "%~dp0"

echo [1/3] Preparing Web Client...
:: Если папка осталась с прошлого раза (сбой), удаляем её
if exist ".\web_client" rmdir /s /q ".\web_client"
:: Копируем свежую версию
xcopy "..\..\web_client" ".\web_client" /E /I /Y >nul

echo [2/3] Searching for logger.exe...
if exist "..\..\build\logger.exe" (
    set EXE_PATH="..\..\build\logger.exe"
) else if exist "..\..\build\Debug\logger.exe" (
    set EXE_PATH="..\..\build\Debug\logger.exe"
) else (
    echo Error: logger.exe not found! Please build the project first.
    :: Чистим за собой перед выходом
    rmdir /s /q ".\web_client"
    pause
    exit /b
)

echo [3/3] Starting Logger Server on COM2...
echo Web Interface: http://localhost:8080
echo (Close this window to stop the server)

:: Запускаем сервер. Скрипт "заснет" здесь, пока работает сервер.
%EXE_PATH% COM6

:: --- Этот код выполнится только после закрытия программы ---
echo.
echo [Cleanup] Removing temporary web files...
if exist ".\web_client" rmdir /s /q ".\web_client"

echo Done.
pause