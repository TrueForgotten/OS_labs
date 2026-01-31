#!/bin/bash

PORT="./ttyReceiver"

if [ -f "../../build/logger" ]; then
    ../../build/logger "$PORT"
else
    echo "Error: ../../build/logger not found!"
fi
