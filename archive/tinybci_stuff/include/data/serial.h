# pragma once

# if defined(_WIN32) || defined(_WIN64)
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
    typedef HANDLE SerialHandle;
# else
#   include <termios.h>
    typedef int SerialHandle;
#   define INVALID_HANDLE_VALUE (-1)

#   ifdef CRTSCTS
#       define TERMIOS_C_ANTIFLAGS (PARENB | CSTOPB | CRTSCTS)
#   else
#       define TERMIOS_C_ANTIFLAGS (PARENB | CSTOPB)
#   endif
# endif

int serialOpen(SerialHandle *, const char *, uint32_t);

int serialWrite(SerialHandle *, uint8_t *, size_t);
int serialRead(SerialHandle *, uint8_t *, size_t);

void serialFlush(SerialHandle *);
void serialClose(SerialHandle *);

void sleepMilliseconds(uint32_t);

# define SERIAL_DATA_WAIT_ITERATION_TIME_MS 100
# define SERIAL_DATA_WAIT_MAXIMUM_ITERATIONS 20
int awaitSerialData(SerialHandle *);