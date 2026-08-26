/*
    Adapted from https://github.com/Marzac/rs232
    as licensed below:

    Cross-platform serial / RS232 library
    Version 0.21, 11/10/2015
    -> WIN32 implementation
    -> rs232-win.c
    
    The MIT License (MIT)

    Copyright (c) 2013-2015 Frédéric Meslin, Florent Touchard
    Email: fredericmeslin@hotmail.com
    Website: www.fredslab.net
    Twitter: @marzacdev

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

# include "data/serial_port_enumeration.h"

# if defined(_WIN32) || defined(_WIN64)
# else
# include <dirent.h>

/** Base name for COM devices */
# if defined(__APPLE__) && defined(__MACH__)
    static const char * deviceBaseNames[] = {
        "tty."
    };
    const static uint8_t BaseNameCount = 1;
# else
    static const char * deviceBaseNames[] = {
        "ttyACM", "ttyUSB", "rfcomm", "ttyS"
    };
    const static uint8_t baseNameCount = 4;
# endif

static char *deviceNames[MAXIMUM_KNOWN_SERIAL_DEVICES];
static uint32_t deviceCount = 0;

/** Private functions */
void appendDevices(const char * baseName)
{
    uint8_t baseNameLength = strlen(baseName);
    struct dirent * dp;
// Enumerate devices
    DIR * dirp = opendir("/dev");
    while ((dp = readdir(dirp)) && deviceCount < MAXIMUM_KNOWN_SERIAL_DEVICES) {
        if (strlen(dp->d_name) >= baseNameLength) {
            if (memcmp(baseName, dp->d_name, baseNameLength) == 0) {
                deviceNames[deviceCount++] = (char *)strdup(dp->d_name);
            }
        }
    }
    closedir(dirp);
}

// ---

uint32_t enumerateSerialPorts()
{
    for (uint32_t i = 0; i < deviceCount; i++) {
        if (deviceNames[i]) free(deviceNames[i]);
        deviceNames[i] = NULL;
    }
    deviceCount = 0;
    for (uint8_t i = 0; i < baseNameCount; i++)
        appendDevices(deviceBaseNames[i]);
    return deviceCount;
}

const char* getSerialPortName(uint32_t index)
{
    if (index >= deviceCount || index < 0) return NULL;
    return deviceNames[index];
}

# endif