# Steps to setup the DSI-7 headset for NeuroMove

## Headset Adjustment

1) Plug the USB connection into the computer and headset.
2) Find port
3) Open DSI Streamer (v1.08.119 or newer)
4) Double click power button on the headset to turn it on.
    - pressing it just once does NOT turn headset on
5) Select port in DSI Streamer and press connect.
    - Need to connect, interrupt, interrupt, connnect to *actually* connect
6) While data is streaming:
    - Diagnostic > Impedance ON
    - A_RESET and adjustment until all electrodes are green.
7) Exit DSI Streamer.

## LSL Streaming setup

Apparently we need to compile the dsi2lsl executable because I can't find the Linux one. Should just need to do this once.

Instructions and source code are [here](https://github.com/Neuroscale-Users/dsi2lsl).

- Launch script `start_dsi2lsl.py` from src folder.
