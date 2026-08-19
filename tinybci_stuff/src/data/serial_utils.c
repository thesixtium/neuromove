# include "data/serial.h"

int awaitSerialData(SerialHandle *handle)
{
    serialFlush(handle);

    uint8_t scanByte;
    uint32_t iterationCount = 0;

    while (!serialRead(handle, &scanByte, 1))
    {
        if (iterationCount++ > SERIAL_DATA_WAIT_MAXIMUM_ITERATIONS) return EXIT_FAILURE;
        sleepMilliseconds(SERIAL_DATA_WAIT_ITERATION_TIME_MS);
    }
    return EXIT_SUCCESS;
}