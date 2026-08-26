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

int seekSerialBytes(SerialHandle *handle, uint8_t *targetBytes, uint8_t targetCount)
{
    uint8_t *scanBuffer = malloc(targetCount);
    uint8_t cursor = 0;
    uint8_t scanAttempts = 200;

    while (scanAttempts-- > 0)
    {
        if (serialRead(handle, scanBuffer + cursor, 1))
        {
            if (scanBuffer[cursor] == targetBytes[cursor])
            {
                if (++cursor == targetCount)
                {
                    free(scanBuffer);
                    return EXIT_SUCCESS;
                }
            }
            else cursor = 0;
        }
    }
    free(scanBuffer);

    fprintf(stderr, "Error: Failed to locate serial start byte\n");
    return EXIT_FAILURE;
}
int seekSerialByte(SerialHandle *handle, uint8_t targetByte)
{
    return seekSerialBytes(handle, &targetByte, 1);
}

// ---

SerialFrame createSerialFrame(uint8_t *pBuffer, uint8_t pLength)
{
    return (SerialFrame) {
        .buffer = pBuffer,
        .length = pLength,
        .cursor = 0
    };
}

void resetSerialFrame(SerialFrame *frame)
{
    for (int i = 0; i < frame->length; i++) frame->buffer[i] = 0;
    frame->cursor = 0;
}

ReadStatus readSerialFrame(SerialHandle *handle, SerialFrame *frame)
{
    int bytesRead = serialRead(
        handle,
        frame->buffer + frame->cursor,
        frame->length - frame->cursor
    );
    if (bytesRead > 0) frame->cursor += bytesRead;

    if (frame->cursor == frame->length) return READ_STATUS_READY;
    else return READ_STATUS_PENDING;
}