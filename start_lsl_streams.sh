#!/bin/bash

tmux kill-session -t neuromove 2>/dev/null
rm -f /tmp/tinybci_full.log

DSI_DIR="$HOME/Documents/neuromove/lib/dsi2lsl-1.0/CLI"
TINYBCI_DIR="$HOME/Documents/neuromove/lib/TinyBCI"
TEST_DIR="$HOME/Documents/neuromove/test"
PORT="/dev/ttyUSB0"
EEG_STREAM="MyEEGStream"

tmux new-session -d -s neuromove -x 220 -y 50
tmux rename-window -t neuromove 'NeuroMove BCI'

# Top left: dsi2lsl
tmux send-keys "cd $DSI_DIR && LD_LIBRARY_PATH=. ./dsi2lsl --port=$PORT --lsl-stream-name=$EEG_STREAM" Enter

# Top right: dummy markers
tmux split-window -h
tmux send-keys "python3 $TEST_DIR/dummy_markers.py" Enter

tmux select-layout tiled
tmux select-pane -t 0
tmux attach-session -t neuromove
