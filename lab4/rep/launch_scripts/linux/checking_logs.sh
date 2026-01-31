#!/bin/bash

# Файлы логов ищем в текущей папке
RAW="temp_raw.log"
HOURLY="temp_hourly.log"
DAILY="temp_daily.log"

while true; do
    clear
    echo "Мониторинг логов"
    echo "================="
    echo ""
    
    echo "--- Сырые данные ---------------------"
    if [ -f "$RAW" ]; then
        cat "$RAW"
    else
        echo "Waiting for data..."
    fi
    echo ""

    echo "--- Среднее за час -------------"
    if [ -f "$HOURLY" ]; then
        cat "$HOURLY"
    else
        echo "No data yet..."
    fi
    echo ""

    echo "--- Среднее за день --------------"
    if [ -f "$DAILY" ]; then
        cat "$DAILY"
    else
        echo "No data yet..."
    fi
    
    echo ""
    echo "================="
        
    sleep 1
done