@echo off
title Sensor Monitoring Dashboard
color 0B

:loop
cls
echo ==========================================
echo        SENSOR MONITORING DASHBOARD        
echo ==========================================
echo.

echo --- RAW LOG (Last 5) ---------------------
if exist temp_raw.log (
    powershell -NoProfile -Command "Get-Content temp_raw.log -Tail 5"
) else (
    echo Waiting for data...
)
echo.

echo --- HOURLY AVERAGES (Last 5) -------------
if exist temp_hourly.log (
    powershell -NoProfile -Command "Get-Content temp_hourly.log -Tail 5"
) else (
    echo No data yet...
)
echo.

echo --- DAILY AVERAGES (Last 5) --------------
if exist temp_daily.log (
    powershell -NoProfile -Command "Get-Content temp_daily.log -Tail 5"
) else (
    echo No data yet...
)

echo.
echo ==========================================
echo Press [Ctrl+C] to exit
timeout /t 1 >nul
goto loop