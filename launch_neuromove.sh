#!/bin/bash

export MESA_GL_VERSION_OVERRIDE=3.3
export MESA_GLSL_VERSION_OVERRIDE=330

# Background helper:
# wait a few seconds, then open main.py in a new terminal
(
    sleep 5

    echo "Opening main.py..."

    x-terminal-emulator \
        -e bash -c 'cd "$HOME/Documents/neuromove"; python3 main.py; echo; echo "main.py stopped"; read'
) &

# Keep TinyBCI completely foregrounded
cd "$HOME/Documents/neuromove/tiny-bci-local-driving/bin" || exit 1

./tiny_bci_local_driving