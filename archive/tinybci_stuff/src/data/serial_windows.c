#include "data/serial.h"

#if defined(_WIN32) || defined(_WIN64)

int serialOpen(SerialHandle *handle, const char *port, uint32_t readTimeout)
{
    *handle = CreateFileA(port,
        GENERIC_READ | GENERIC_WRITE, 0, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL
    );
    if (*handle == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Failed to open serial port %s\n", port);
        return EXIT_FAILURE;
    }

    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);
    GetCommState(*handle, &dcb);
    dcb.BaudRate = CBR_115200;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity = NOPARITY;
    SetCommState(*handle, &dcb);

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = readTimeout;
    SetCommTimeouts(*handle, &timeouts);

    return EXIT_SUCCESS;
}

// ---

int serialWrite(SerialHandle *handle, uint8_t *buffer, size_t bufferLength)
{
    DWORD writeCount = 0;
    WriteFile(*handle, buffer, (DWORD)bufferLength, &writeCount, NULL);
    return writeCount;
}

int serialRead(SerialHandle *handle, uint8_t *buffer, size_t bufferLength)
{
    DWORD readCount = 0;
    ReadFile(*handle, buffer, (DWORD)bufferLength, &readCount, NULL);
    return readCount;
}

void serialFlush(SerialHandle *handle) { PurgeComm(*handle, PURGE_RXCLEAR); }
void serialClose(SerialHandle *handle) { CloseHandle(*handle); }

// ---

void sleepMilliseconds(uint32_t ms) { Sleep(ms); }

#endif