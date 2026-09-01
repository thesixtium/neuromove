# NeuroMove

NeuroMove is a brain-computer interface (BCI) power mobility trainer designed to allow a user to control powered mobility using EEG-based commands while maintaining a separate safety and motor-control layer on a Raspberry Pi and Arduino.

This repository contains the software used to integrate:

* a Wearable Sensing DSI EEG headset,
* a TinyBCI-based SSVEP classifier,
* a Raspberry Pi 4,
* an Arduino Uno,
* the wheelchair motor relay interface,
* ultrasonic and force sensors,
* and the existing NeuroMove navigation / driving framework.

The current development version is on the `ashdawg` branch.

> **Important:** This is research software under active development. It should not be treated as a certified medical-device or mobility safety system. Test motor-control changes with the wheelchair safely supported / unloaded before testing with a user.

---

# Table of Contents

1. [System Overview](#system-overview)
2. [Current System Architecture](#current-system-architecture)
3. [Repository Structure](#repository-structure)
4. [Hardware](#hardware)
5. [BCI Local Driving](#bci-local-driving)
6. [DSI EEG Integration](#dsi-eeg-integration)
7. [Shared Memory Interface](#shared-memory-interface)
8. [Motor Control](#motor-control)
9. [Arduino and Sensor Wiring](#arduino-and-sensor-wiring)
10. [Software Setup](#software-setup)
11. [Building TinyBCI](#building-tinybci)
12. [Running NeuroMove](#running-neuromove)
13. [Testing Without the Full System](#testing-without-the-full-system)
14. [Troubleshooting](#troubleshooting)
15. [Known Issues / Technical Debt](#known-issues--technical-debt)
16. [Historical / Archived Components](#historical--archived-components)
17. [Future Development](#future-development)

---

# System Overview

The original NeuroMove architecture used multiple computers and software components to perform BCI processing and mobility control.

The Summer 2026 work focused on simplifying the system so that **local SSVEP driving can run directly on the Raspberry Pi**, removing the need for the LattePanda computer in the local-driving pipeline.

The intended local-driving flow is:

```text
User looks at SSVEP stimulus
          │
          ▼
      DSI headset
          │
      EEG over serial
          │
          ▼
 tiny-bci-local-driving
          │
   TinyBCI inference
          │
          ▼
 POSIX shared memory
     ("pookinator")
          │
          ▼
       main.py
          │
          ▼
      Driving.py
          │
          ▼
    ArduinoUno.py
          │
      USB serial
          │
          ▼
      Arduino Uno
          │
          ▼
       Relays
          │
          ▼
Wheelchair motor controller
```

The Raspberry Pi therefore acts as the central computer for both BCI inference and wheelchair control.

---

# Current System Architecture

## Raspberry Pi

The Raspberry Pi runs two main processes during BCI local driving:

### 1. TinyBCI

Located in:

```text
tiny-bci-local-driving/
```

This program:

1. connects directly to the DSI headset,
2. acquires EEG,
3. presents the SSVEP stimuli,
4. processes EEG through TinyBCI,
5. determines the selected target,
6. writes the final selection into POSIX shared memory.

### 2. NeuroMove Python controller

Started using:

```bash
python3 main.py
```

`main.py` connects to the BCI shared-memory output and initializes the NeuroMove driving system.

The relevant control path is:

```text
main.py
   ↓
src/RaspberryPi/Driving.py
   ↓
src/Arduino/ArduinoUno.py
   ↓
src/Arduino/Arduino.ino
   ↓
motor relays
```

---

# Repository Structure

At a high level:

```text
neuromove/
│
├── main.py
│
├── run_tinybci.sh
├── start_lsl_streams.sh
│
├── src/
│   ├── Arduino/
│   │   ├── Arduino.ino
│   │   ├── ArduinoUno.py
│   │   └── ...
│   │
│   └── RaspberryPi/
│       ├── Driving.py
│       ├── SharedMemory.py
│       ├── States.py
│       └── ...
│
├── test/
│   └── RaspberryPi/
│       └── ...
│
├── tiny-bci-local-driving/
│   ├── CMakeLists.txt
│   ├── assets/
│   ├── cmake/
│   ├── include/
│   ├── src/
│   └── thirdparty/
│
├── lib/
│
└── archive/
```

## `main.py`

Entry point for the current Python driving system.

It connects to the shared-memory object created by TinyBCI:

```text
pookinator
```

and passes BCI selections to the driving system.

## `src/RaspberryPi/`

Contains Raspberry Pi-side control code.

Important files include:

### `Driving.py`

Controls execution of movement commands.

It handles:

* local driving,
* destination-driving command strings,
* motor timing,
* stopping after movement,
* communication with the Arduino interface.

### `SharedMemory.py`

Python interface to the POSIX shared-memory regions used for communication between processes.

### `States.py`

Defines system states and motor directions.

Current high-level states include:

```text
START
SETUP
LOCAL
DESTINATION
RECOVERY
OFF
```

Destination-driving states include:

```text
IDLE
MAP_ROOM
SELECT_DESTINATION
TRANSLATE_TO_MOVEMENT
DRIVE
```

## `src/Arduino/`

Contains the Arduino firmware and Python serial interface.

## `tiny-bci-local-driving/`

Modified TinyBCI SSVEP application used for NeuroMove local driving.

This directory contains both the BCI pipeline and stimulus presentation code.

It builds two executables:

```text
tiny_bci_local_driving
headless_tiny_bci_local_driving
```

The normal executable includes the graphical SSVEP presentation.

The headless executable excludes the presentation layer and is useful for development/testing where a graphical display is not required.

## `archive/`

Contains previous or unused NeuroMove components retained for reference.

These should generally **not** be assumed to represent the current local-driving architecture.

---

# Hardware

The Summer 2026 system uses:

| Component                           | Purpose                               |
| ----------------------------------- | ------------------------------------- |
| Raspberry Pi 4                      | Main NeuroMove computer               |
| Arduino Uno                         | Low-level relay / sensor interface    |
| Wearable Sensing DSI headset        | EEG acquisition                       |
| HDMI touchscreen                    | SSVEP stimulus presentation           |
| Relay interface                     | Interfaces with wheelchair controller |
| 5 × MaxBotix ultrasonic sensors     | Obstacle sensing                      |
| FSR                                 | Physical / safety input               |
| Wheelchair / power mobility trainer | Mobility platform                     |

The current local BCI pipeline no longer requires the LattePanda.

---

# BCI Local Driving

## Overview

NeuroMove local driving uses **steady-state visually evoked potentials (SSVEPs)**.

Several visual targets flicker at different frequencies. The user focuses attention on the target corresponding to the desired movement.

TinyBCI processes the EEG and determines which stimulus frequency is most strongly represented.

That classification is translated into a driving command.

The local-driving interface is designed around:

```text
FORWARD
LEFT
RIGHT
STOP
```

The current implementation uses trials rather than continuously issuing a new motor command from every individual inference.

A trial consists of a period of SSVEP stimulation followed by selection of a final command.

---

# DSI EEG Integration

## Direct serial acquisition

The current TinyBCI implementation includes a direct Wearable Sensing DSI interface:

```text
tiny-bci-local-driving/src/data/dsi_eeg_source.c
```

This replaces the earlier architecture:

```text
DSI headset
    ↓
dsi2lsl
    ↓
LSL
    ↓
TinyBCI
```

with:

```text
DSI headset
    ↓
DSI API
    ↓
TinyBCI
```

This was done because TinyBCI operated correctly with synthetic EEG while the LSL-based configuration produced instability on the Raspberry Pi.

## Acquisition thread

The DSI API performs EEG acquisition in the background.

When a new sample becomes available, the DSI sample callback:

1. reads each selected DSI channel,
2. creates a timestamp,
3. pushes the sample directly into the TinyBCI input pipeline.

Conceptually:

```text
DSI background acquisition
          │
          ▼
   dsiSampleCallback()
          │
          ▼
DSI_Channel_ReadBuffered()
          │
          ▼
   in_push_signal()
          │
          ▼
     TinyBCI pipeline
```

This keeps serial acquisition separate from the graphical rendering loop.

## DSI API files

The DSI integration requires Wearable Sensing's DSI API, including files such as:

```text
DSI.h
DSI_API_Loader.c
libDSI.so
```

The Raspberry Pi requires an **ARM64-compatible** DSI library.

The version tested during development was DSI API **v1.21.3**, supplied with compatibility for the Raspberry Pi's GLIBC version.

## Serial port

The DSI headset has commonly appeared as:

```text
/dev/ttyUSB0
```

However, **do not assume the port number**.

Check using:

```bash
ls /dev/ttyUSB*
ls /dev/ttyACM*
```

before starting the system.

The Arduino has commonly appeared as:

```text
/dev/ttyACM0
```

---

# DSI Montage / Headset Compatibility

The EEG montage is important.

TinyBCI requests channels by electrode name. The DSI API then attempts to match those names to channels available on the connected headset.

This means that a montage configured for one DSI headset may **not automatically work with another headset**.

For example, the DSI-7 Flex may expose channel names differently from another DSI configuration.

A typical error looks like:

```text
Failed to match desired source ...
```

If this occurs, inspect:

```text
tiny-bci-local-driving/src/data/dsi_eeg_source.c
```

and the EEG-source selection/configuration code to determine which montage is being passed into:

```c
DSI_Headset_ChooseChannels(...)
```

Do not assume that changing the physical headset is sufficient. The software montage must match the channel names reported by that headset.

---

# Shared Memory Interface

POSIX shared memory is used to communicate between independently running C and Python processes.

The Python wrapper is:

```text
src/RaspberryPi/SharedMemory.py
```

## BCI selection

The current `main.py` connects to:

```text
pookinator
```

TinyBCI writes the selected target into this shared-memory region.

Python then reads it using:

```python
read_local_driving()
```

The current mapping in `SharedMemory.py` is:

```text
"1" → FORWARD
"2" → RIGHT
"3" → LEFT
other → STOP
```

Therefore the complete control flow is:

```text
TinyBCI classification
       ↓
target number
       ↓
"pookinator" shared memory
       ↓
SharedMemory.read_local_driving()
       ↓
MotorDirections
       ↓
Driving.drive_one_unit()
```

Other shared-memory objects used by the wider NeuroMove code include:

```text
driving_direction
local_driving
directions
```

Additional shared-memory interfaces may exist elsewhere in the legacy / destination-driving code.

---

# Motor Control

Motor directions are defined in:

```text
src/RaspberryPi/States.py
```

The current enum is:

```python
class MotorDirections(Enum):
    FORWARD = b"w"
    BACKWARD = b"s"
    LEFT = b"a"
    RIGHT = b"d"
    STOP = b"x"
```

`Driving.py` passes these commands to:

```text
ArduinoUno.send_direction()
```

which sends the corresponding byte over serial.

## Movement timing

Current timing parameters in `Driving.py` are:

```python
self.t_accel = 0.28
self.t_const = 1.15

self.t_rotaccel = 0.28
self.t_rotconst = 0.77
```

Forward/backward movements and rotations therefore use different timing constants.

After executing a movement, `Driving.py` explicitly sends:

```text
STOP
```

to the Arduino.

These values were empirically determined and should be recalibrated if:

* the mobility platform changes,
* motor speed changes,
* relay behaviour changes,
* battery voltage significantly changes,
* or the desired movement increment changes.

---

# Arduino and Sensor Wiring

## Motor relay pins

The Arduino firmware currently defines:

```text
D2 → Right
D3 → Left
D4 → Reverse
D5 → Forward
```

These outputs control the relay interface to the wheelchair controller.

## Ultrasonic sensors

Five ultrasonic inputs are assigned:

```text
D7
D8
D9
D10
D11
```

with a shared trigger on:

```text
D6
```

The hardware used during development was the MaxBotix LV-MaxSonar-EZ0 / MB1000 family.

The sensors use pulse-width output.

A longer `pulseIn()` timeout may be required when reading multiple sensors sequentially. During testing, increasing the timeout from approximately:

```text
50,000 µs
```

to:

```text
100,000 µs
```

allowed later sensors in the sequence to return correctly.

## Force sensor

The FSR is connected to:

```text
A0
```

The exact FSR wiring should be verified before relying on it as a safety input.

---

# Software Setup

The system has primarily been developed on a Raspberry Pi 4 running 64-bit Linux.

## Clone the repository

```bash
cd ~/Documents

git clone https://github.com/thesixtium/neuromove.git

cd neuromove

git checkout ashdawg
```

## Python dependencies

The Python side requires packages including:

```text
numpy
pandas
scipy
pyserial
pyduinocli
```

Install them using the appropriate Python environment, for example:

```bash
pip3 install numpy pandas scipy pyserial pyduinocli
```

Additional dependencies may be required by destination-driving or archived components.

## Arduino CLI

`ArduinoUno.py` expects the Arduino CLI at:

```text
./src/Arduino/arduino-cli
```

When `ArduinoUno` is initialized, it currently:

1. detects Arduino boards,
2. compiles `Arduino.ino`,
3. uploads it to the Arduino Uno,
4. opens the serial connection.

The default Arduino serial configuration is:

```text
Port: /dev/ttyACM0
Baud: 19200
```

If the Arduino appears on another port, update the configuration accordingly.

---

# Building TinyBCI

The TinyBCI project is located at:

```bash
cd ~/Documents/neuromove/tiny-bci-local-driving
```

The project uses CMake.

## Dependencies

The graphical application requires OpenGL/X11-related dependencies and Raylib.

Packages required during Raspberry Pi setup included libraries for:

```text
X11
Xi
Xcursor
Xrandr
OpenGL
```

For example, depending on the Raspberry Pi OS version:

```bash
sudo apt update

sudo apt install \
    cmake \
    build-essential \
    libx11-dev \
    libxi-dev \
    libxcursor-dev \
    libxrandr-dev \
    libgl1-mesa-dev
```

## Configure

From `tiny-bci-local-driving/`:

```bash
mkdir -p build
cd build
cmake ..
```

## Compile

```bash
cmake --build .
```

or:

```bash
make -j4
```

The executables are configured to be placed in:

```text
tiny-bci-local-driving/bin/
```

The main executable should therefore be:

```text
tiny-bci-local-driving/bin/tiny_bci_local_driving
```

---

# Raspberry Pi OpenGL Compatibility

On the Raspberry Pi, Raylib/OpenGL may require Mesa version overrides.

Before starting TinyBCI:

```bash
export MESA_GL_VERSION_OVERRIDE=3.3
export MESA_GLSL_VERSION_OVERRIDE=330
```

Without these, errors may include:

```text
GLX: Failed to create context
GLXBadFBConfig
Failed to initialize Window
Failed to initialize platform
```

These errors can also occur when trying to launch the graphical application from an environment that does not have access to the active desktop/display session.

For this reason, TinyBCI should initially be tested by launching it manually from a terminal opened inside the Raspberry Pi desktop session.

---

# Running NeuroMove

## Before starting

Connect:

1. Raspberry Pi display,
2. DSI headset,
3. Arduino Uno,
4. relay interface,
5. required NeuroMove sensors.

Check serial devices:

```bash
ls /dev/ttyUSB*
ls /dev/ttyACM*
```

Typical configuration during development:

```text
DSI headset → /dev/ttyUSB0
Arduino     → /dev/ttyACM0
```

Verify this each time.

---

## Terminal 1 — Start TinyBCI

```bash
cd ~/Documents/neuromove/tiny-bci-local-driving/bin
```

Set the Mesa overrides:

```bash
export MESA_GL_VERSION_OVERRIDE=3.3
export MESA_GLSL_VERSION_OVERRIDE=330
```

Then run:

```bash
./tiny_bci_local_driving
```

TinyBCI may display a list of serial ports and request the DSI headset port.

During development the correct option was frequently:

```text
2
```

but **select the port corresponding to the DSI headset rather than assuming that option 2 will always be correct.**

Once connected, the SSVEP presentation window should appear.

---

## Terminal 2 — Start NeuroMove

From the repository root:

```bash
cd ~/Documents/neuromove
python3 main.py
```

This initializes:

* the shared-memory reader,
* `Driving`,
* the Arduino interface,
* and the motor-control system.

---

## Starting a BCI session

Once both programs are running:

1. confirm the DSI headset is connected,
2. confirm TinyBCI is receiving EEG,
3. confirm the Arduino has initialized,
4. make sure the mobility platform is safe to move,
5. focus the TinyBCI stimulus window,
6. press **SPACE** to begin the SSVEP trials.

TinyBCI should classify the user's selection and write it to shared memory.

Python then converts that selection into a movement command.

---

# Testing Without the Full System

When debugging, test each layer separately rather than immediately running the entire BCI → wheelchair pipeline.

A useful order is:

```text
1. Relay / Arduino test
        ↓
2. Python → Arduino test
        ↓
3. Shared-memory test
        ↓
4. TinyBCI with synthetic EEG
        ↓
5. TinyBCI with DSI EEG
        ↓
6. TinyBCI + Python
        ↓
7. Full wheelchair test
```

This makes it much easier to determine which subsystem is responsible for a failure.

## Test files

Hardware and Raspberry Pi test programs are stored under:

```text
test/
```

including Raspberry Pi / hardware bench-testing utilities.

Use these before modifying the main driving system whenever possible.

---

# Troubleshooting

## TinyBCI immediately exits

Run it manually from a desktop terminal:

```bash
cd ~/Documents/neuromove/tiny-bci-local-driving/bin

export MESA_GL_VERSION_OVERRIDE=3.3
export MESA_GLSL_VERSION_OVERRIDE=330

./tiny_bci_local_driving
```

Read the terminal output before attempting to automate startup.

---

## `GLXBadFBConfig`

Example:

```text
GLFW: Error: 65543
GLX: Failed to create context: GLXBadFBConfig
```

Try:

```bash
export MESA_GL_VERSION_OVERRIDE=3.3
export MESA_GLSL_VERSION_OVERRIDE=330
```

Also ensure the program is being launched from the Raspberry Pi's active graphical desktop session.

Scripts launched from another TTY/session may not inherit the correct graphical environment.

---

## DSI headset does not appear

Check:

```bash
ls /dev/ttyUSB*
ls /dev/ttyACM*
```

Then:

```bash
dmesg | tail -50
```

Unplug and reconnect the headset and compare the device list.

Do not confuse the Arduino's serial port with the DSI headset port.

---

## DSI connects but TinyBCI reports channel errors

An error such as:

```text
Failed to match desired source ...
```

usually means that the requested montage does not match the channel names exposed by the connected DSI headset.

This is particularly important when switching between DSI headset models such as a standard DSI-7 configuration and the DSI Flex.

Inspect the montage passed to:

```c
DSI_Headset_ChooseChannels(...)
```

and compare it against the source names reported by the headset.

---

## TinyBCI works with synthetic EEG but not DSI EEG

This is a useful diagnostic result.

It suggests that:

* the presentation system works,
* the TinyBCI processing pipeline can run,
* and the problem is likely upstream in EEG acquisition/configuration.

Check:

```text
DSI serial connection
DSI API library
DSI montage
channel count
sample rate
DSI API errors
```

before changing the TinyBCI classifier.

---

## `Failed to update Tiny BCI Pipeline | code: -5`

This was encountered during earlier LSL-based testing.

Synthetic EEG worked while the external EEG configuration failed.

The local-driving system was subsequently moved toward direct DSI serial acquisition rather than relying on:

```text
DSI → dsi2lsl → LSL → TinyBCI
```

If this error appears again, first determine whether the program is using the legacy LSL path or the current direct DSI path.

---

## Arduino not found

Check:

```bash
ls /dev/ttyACM*
```

The expected port has commonly been:

```text
/dev/ttyACM0
```

but it may change.

Also check:

```bash
arduino-cli board list
```

---

## Permission denied on serial port

Check:

```bash
ls -l /dev/ttyACM0
ls -l /dev/ttyUSB0
```

The user may need to belong to the appropriate serial-device group, commonly:

```bash
sudo usermod -a -G dialout $USER
```

Log out/reboot after changing group membership.

---

## BCI selection appears but wheelchair does not move

Debug downward through the control chain:

```text
Did TinyBCI classify?
        ↓
Did shared memory change?
        ↓
Did Python read the selection?
        ↓
Did Driving.py receive the correct MotorDirections value?
        ↓
Did ArduinoUno.py send a serial byte?
        ↓
Did Arduino.ino receive it?
        ↓
Did the correct relay activate?
        ↓
Did the wheelchair controller respond?
```

Do not change the classifier simply because a motor failed to move.

---

# Known Issues / Technical Debt

The `ashdawg` branch is an active development branch and currently contains some inconsistencies that should be cleaned up before treating the repository as reproducibly deployable.

## 1. DSI library path is hardcoded to an old directory

In:

```text
tiny-bci-local-driving/src/data/dsi_eeg_source.c
```

the DSI API is currently loaded using a hardcoded path containing:

```text
/home/pi/Documents/tiny-bci-ssvep-experiment/...
```

However, the project has since been moved to:

```text
~/Documents/neuromove/tiny-bci-local-driving/
```

This path should be corrected.

Preferably, the DSI library location should eventually be configured through CMake or a runtime environment variable rather than hardcoded into the C source.

---

## 2. DSI montage is not automatically headset-independent

The DSI API receives a requested montage from TinyBCI.

Different DSI headset configurations may expose different channel names.

Therefore switching from one DSI headset to another — particularly to the DSI Flex — may require updating the requested montage.

A future improvement would be to:

1. query the connected headset model,
2. list available source names,
3. select an appropriate montage dynamically or through a configuration file.

---

## 3. Python and Arduino motor command protocols need to be reconciled

`States.py` currently defines:

```text
FORWARD  = w
BACKWARD = s
LEFT     = a
RIGHT    = d
STOP     = x
```

However, the current `Arduino.ino` contains older command decoding corresponding to values historically used for:

```text
G → Forward
C → Reverse
1 → Left
4 → Right
```

These interfaces should be unified so that **one command protocol is defined in one place and used consistently by both Python and Arduino**.

Until this is resolved, verify motor commands using a bench test before relying on `main.py` for mobility control.

---

## 4. `run_tinybci.sh` contains old paths

The current script still references the previous directory/executable:

```text
~/Documents/tiny-bci-ssvep-experiment/bin
```

and:

```text
tiny_bci_ssvep_experiment
```

The current project is:

```text
~/Documents/neuromove/tiny-bci-local-driving/
```

with executable:

```text
tiny_bci_local_driving
```

Update the script before using it as the normal startup method.

For now, manual startup from the desktop terminal is the most transparent debugging method.

---

## 5. Startup automation is not yet fully reliable

Attempts were made to automate:

```text
TinyBCI startup
→ serial-port selection
→ main.py startup
```

Using terminal multiplexing / scripting caused graphical-context problems on the Raspberry Pi, including:

```text
GLXBadFBConfig
```

Automating stdin for the TinyBCI port menu also caused repeated port-selection behaviour.

Until this is cleaned up, the reliable procedure is to run TinyBCI and `main.py` in separate terminals.

---

## 6. Shared-memory naming should be cleaned up

The BCI shared-memory object currently used by `main.py` is named:

```text
pookinator
```

This works as an internal development name but should eventually be renamed to something descriptive, for example:

```text
bci_selection
```

Both the C producer and Python consumer must be changed together.

---

## 7. Shared-memory ownership / cleanup requires care

Both C and Python access POSIX shared memory.

If a process crashes without properly cleaning up its shared-memory region, a later process may connect to stale memory.

When debugging unexpected shared-memory behaviour, inspect:

```bash
ls /dev/shm
```

and confirm which process is responsible for creating each shared-memory object.

---

## 8. Motor timing requires further calibration

Movement duration is currently time-based.

This means that a command represents approximately a fixed movement increment rather than closed-loop position control.

Turning and forward-distance calibration should be verified experimentally before use.

---

## 9. Impedance checking is not currently integrated

An impedance-checking interface was investigated using the Wearable Sensing DSI API.

The DSI impedance driver was able to enter impedance mode, but valid per-channel impedance values were not reliably obtained during development.

Because the DSI impedance driver injects impedance-test signals and should not operate simultaneously with normal EEG acquisition, impedance testing should be implemented as a separate **pre-session setup step**, not during active BCI classification.

Experimental impedance code should not be reintroduced into the acquisition path until it has been independently validated.

---

# Historical / Archived Components

Some repository files represent earlier NeuroMove architectures.

In particular, local driving previously relied on:

```text
DSI headset
      ↓
Windows / LattePanda
      ↓
dsi2lsl
      ↓
Lab Streaming Layer
      ↓
Raspberry Pi TinyBCI
```

During Summer 2026 this was replaced with direct DSI acquisition on the Raspberry Pi:

```text
DSI headset
      ↓
serial
      ↓
Wearable Sensing DSI API
      ↓
TinyBCI
```

This distinction is important when reading older scripts or documentation.

Files involving:

```text
dsi2lsl
LSL startup scripts
LattePanda
old TinyBCI directories
```

may therefore represent previous approaches rather than the intended current local-driving pipeline.

The `archive/` directory exists to retain older code without implying that it belongs to the active system.

---

# Future Development

Major next steps include:

### Stabilize local BCI driving

* reconcile Arduino and Python motor commands,
* remove stale hardcoded paths,
* verify DSI headset montage configuration,
* validate left/right/forward commands independently,
* calibrate movement duration,
* ensure every movement is followed reliably by STOP.

### Improve TinyBCI presentation

* optimize stimulus placement for the 1024 × 600 touchscreen,
* use equal-size SSVEP targets where possible,
* maximize target size while maintaining sufficient visual separation,
* maintain reliable timing,
* align classification output with trial completion.

### Improve BCI decision logic

The local-driving interface should prioritize avoiding unintended movements.

Useful safeguards include:

* confidence thresholds,
* majority voting across inference windows,
* minimum-vote requirements,
* returning **no movement / STOP** when evidence is insufficient.

### Improve DSI compatibility

* remove hardcoded DSI library paths,
* detect headset configuration,
* make montage selection configurable,
* test DSI Flex support,
* investigate a standalone pre-session impedance check.

### Improve startup

Eventually the desired startup experience is approximately:

```text
Start NeuroMove
      ↓
detect Arduino
      ↓
detect DSI headset
      ↓
initialize TinyBCI
      ↓
perform setup / signal checks
      ↓
open SSVEP interface
      ↓
start local driving
```

without requiring multiple terminals or manual serial-port selection.

### Safety

Before clinical or participant use, verify:

* STOP behaviour,
* motor command timeout behaviour,
* relay fail-safe behaviour,
* sensor operation,
* BCI no-selection behaviour,
* loss-of-EEG behaviour,
* loss-of-serial behaviour,
* process-crash behaviour,
* emergency stopping,
* command duration limits.

A software crash or loss of BCI input should never result in sustained unintended motor activation.

---

# Development Notes

When debugging the system, avoid changing multiple layers simultaneously.

The most useful principle for NeuroMove development is:

```text
BCI
 ↓
shared memory
 ↓
Python
 ↓
serial
 ↓
Arduino
 ↓
relay
 ↓
motor
```

Test the interfaces individually.

For example, if TinyBCI prints `RIGHT` but the wheelchair does not turn right, first verify that:

```text
TinyBCI → shared memory
```

is correct before modifying TinyBCI itself.

Then verify:

```text
shared memory → Python
Python → serial
serial → Arduino
Arduino → relay
```

in order.

This approach makes failures much easier to isolate.

---

# Branch

Current Summer 2026 development branch:

```bash
git checkout ashdawg
```

Repository:

`thesixtium/neuromove`

The `ashdawg` branch contains the Raspberry Pi / TinyBCI local-driving integration developed during Summer 2026.
