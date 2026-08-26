/*
    Adapted from https://github.com/Marzac/rs232
    as licensed below:

    Cross-platform serial / RS232 library
    Version 0.21, 11/10/2015
    -> LINUX and MacOS implementation
    -> rs232-linux.c

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

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

# include "data/serial_port_enumeration.h"

# if defined(_WIN32) || defined(_WIN64)

static int32_t enumeratedPorts[MAXIMUM_KNOWN_SERIAL_DEVICES];
static uint32_t deviceCount = 0;

# define COM_MINDEVNAME 16384
const char * portNamePattern = "COM???";

/** Windows system constants */
# define ERROR_INSUFFICIENT_BUFFER   122

/** Windows system functions */
uint32_t __stdcall GetLastError(void);
void __stdcall SetLastError(uint32_t dwErrCode);

uint32_t  __stdcall QueryDosDeviceA(const char * lpDeviceName, char * lpTargetPath, uint32_t ucchMax);

// ---

const char * findPattern(const char * string, const char * pattern, int * value)
{
    char c, n = 0;
    const char * sp = string;
    const char * pp = pattern;
// Check for the string pattern
    while (1) {
        c = *sp ++;
        if (c == '\0') {
            if (*pp == '?') break;
            if (*sp == '\0') break;
            n = 0;
            pp = pattern;
        }else{
            if (*pp == '?') {
            // Expect a digit
                if (c >= '0' && c <= '9') {
                    n = n * 10 + (c - '0');
                    if (*pp ++ == '\0') break;
                }else{
                    n = 0;
                    pp = portNamePattern;
                }
            }else{
            // Expect a character
                if (c == *pp) {
                    if (*pp ++ == '\0') break;
                }else{
                    n = 0;
                    pp = portNamePattern;
                }
            }
        }
    }
// Return the value
    * value = n;
    return sp;
}

// ---

uint32_t enumerateSerialPorts()
{
// Get devices information text
    uint32_t size = COM_MINDEVNAME;
    char * list = (char *) malloc(size);
    SetLastError(0);
    QueryDosDeviceA(NULL, list, size);
    while (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        size *= 2;
        char * nlist = realloc(list, size);
        if (!nlist) {
            free(list);
            return 0;
        }
        list = nlist;
        SetLastError(0);
        QueryDosDeviceA(NULL, list, size);
    }
// Gather all COM ports
    int port;
    const char * nlist = findPattern(list, portNamePattern, &port);
    deviceCount = 0;
    while(port > 0 && deviceCount < MAXIMUM_KNOWN_SERIAL_DEVICES) {
        enumeratedPorts[deviceCount++] = port;
        nlist = findPattern(nlist, portNamePattern, &port);
    }
    free(list);
    return deviceCount;
}

const char* getSerialPortName(uint32_t index)
{
    static char name[MAXIMUM_PORT_NAME_LENGTH];
    if (index < 0 || index >= deviceCount)
        return 0;
    sprintf(name, "COM%i", enumeratedPorts[index]);
    return name;
}

# endif