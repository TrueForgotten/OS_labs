#!/bin/bash

PORT="./ttySender"

if [ -f "../../build/emulator" ]; then
    ../../build/emulator "$PORT"
else
    echo "Error: ../../build/emulator not found!"
fi
