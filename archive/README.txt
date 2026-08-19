# This file is part of the Application Programmer's Interface (API) for Dry Sensor Interface
# (DSI) EEG systems by Wearable Sensing. The API consists of code, headers, dynamic libraries
# and documentation.  The API allows software developers to interface directly with DSI
# systems to control and to acquire data from them.
# 
# The API is not certified to any specific standard. It is not intended for clinical use.
# The API, and software that makes use of it, should not be used for diagnostic or other
# clinical purposes.  The API is intended for research use and is provided on an "AS IS"
# basis.  WEARABLE SENSING, INCLUDING ITS SUBSIDIARIES, DISCLAIMS ANY AND ALL WARRANTIES
# EXPRESSED, STATUTORY OR IMPLIED, INCLUDING BUT NOT LIMITED TO ANY IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, NON-INFRINGEMENT OR THIRD PARTY RIGHTS.
# 
# Copyright (c) @YEARS@ Wearable Sensing LLC



The Dry Sensor Interface API dynamic libraries in this directory have version @VERSION@.

DSI API - Platform Library Reference
=====================================

Each platform has a corresponding pre-built dynamic library in this folder.
Load the correct one for your operating system and architecture.

  libDSI-Windows-x64.dll        Windows 64-bit (x86_64)
                                 Visual Studio 2010+, Windows 7 and later

  libDSI-Windows-Win32.dll      Windows 32-bit (x86)
                                 For legacy 32-bit applications on Windows

  libDSI-Windows-ARM64.dll      Windows ARM64
                                 Surface Pro X, Snapdragon-based Windows PCs

  libDSI-Linux-x86_64.so        Linux 64-bit (x86_64)
                                 Ubuntu, Debian, and other x64 distributions

  libDSI-Linux-arm64.so         Linux ARM64
                                 Raspberry Pi 4/5, NVIDIA Jetson, and other
                                 64-bit ARM Linux systems

  libDSI-Linux-armv7l.so        Linux ARM32 (ARMv7)
                                 Raspberry Pi 2 and 3
                                 NOTE: For original Pi Zero (ARMv6), this
                                 binary may not run. Contact support if needed.

  libDSI-Darwin-arm64.dylib     macOS Apple Silicon (M1, M2, M3, M4)
                                 macOS 11 (Big Sur) and later

  libDSI-Darwin-x86_64.dylib    macOS Intel (x86_64)
                                 macOS 10.9 (Mavericks) and later


To create a DSI application:
----------------------------

(1) Add the file DSI_API_Loader.c (C/C++) or DSI.py (Python) to your project to load the correct library
at runtime. Pass the library filename as the first argument, or set the DSI_PLATFORM environment variable.

(2) #include "DSI.h" in your own C or C++ code

(3) Call Load_DSI_API() in your code before calling any of the other functions.

(4) Check the return value of Load_DSI_API before proceeding. It will be zero on
    success. Any non-zero value means the API failed to load and other API calls
    (function names starting with DSI_) should not be called.
    
Consult demo.c for more details, including commented example code.

Support:
--------
Wearable Sensing - support@wearablesensing.com

See LICENSE.md for license and warranty terms.

