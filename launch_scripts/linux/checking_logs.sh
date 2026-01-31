#!/bin/bash

# Файлы логов ищем в текущей папке
RAW="temp_raw.log"
HOURLY="temp_hourly.log"
DAILY="temp_daily.log"

while true; do
    clear
    echo "=========================================="
    echo "       SENSOR MONITORING DASHBOARD        "
    echo "=========================================="
    echo ""
    
    echo "--- RAW LOG (Last 5 sec) -----------------"
    if [ -f "$RAW" ]; then
        tail -n 5 "$RAW"
    else
        echo "Waiting for file..."
    fi
    echo ""

    echo "--- HOURLY AVERAGES (Last 5) -------------"
    if [ -f "$HOURLY" ]; then
        tail -n 5 "$HOURLY"
    else
        echo "No data yet..."
    fi
    echo ""

    echo "--- DAILY AVERAGES (Last 5) --------------"
    if [ -f "$DAILY" ]; then
        tail -n 5 "$DAILY"
    else
        echo "No data yet..."
    fi
    
    echo ""
    echo "=========================================="
    echo "Press [Ctrl+C] to exit monitor"
    
    sleep 1
done
