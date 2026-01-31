@echo off
chcp 65001 >nul
title Sensor Monitoring Dashboard
color 0B

:loop
cls
echo Мониторинг логов        
echo =================
echo.

echo --- Сырые данные ---------------------
if exist temp_raw.log (
    type temp_raw.log
) else (
    echo Waiting for data...
)
echo.

echo --- Среднее за час -------------
if exist temp_hourly.log (
    type temp_hourly.log
) else (
    echo No data yet...
)
echo.

echo --- Среднее за день --------------
if exist temp_daily.log (
    type temp_daily.log
) else (
    echo No data yet...
)

echo.
echo =================
timeout /t 1 >nul
goto loop