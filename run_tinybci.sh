#!/bin/bash

DSI_DIR="$HOME/Documents/neuromove/lib/dsi2lsl-1.0/CLI"
TINYBCI_DIR="$HOME/Documents/tiny-bci-ssvep-experiment/bin"

PORT="/dev/ttyUSB0"
EEG_STREAM="MyEEGStream"

echo "Checking for DSI headset..."

if [ ! -e "$PORT" ]; then
    echo "ERROR: No headset found at $PORT"
    exit 1
fi

echo "Starting TinyBCI..."

export MESA_GL_VERSION_OVERRIDE=3.3
export MESA_GLSL_VERSION_OVERRIDE=330

cd "$TINYBCI_DIR" || exit 1

./tiny_bci_ssvep_experiment

echo "TinyBCI closed."

echo "Stopping dsi2lsl..."
kill "$DSI_PID" 2>/dev/null

echo "Done."
