# include "data/unicorn_eeg_source.h"
# include "data/serial.h"
# include "storage.h"
# include "microsecond_timer.h"

static SerialHandle handle = INVALID_HANDLE_VALUE;

static uint8_t frameBuffer[UNICORN_PACKET_SIZE];
static SerialFrame frame;

static float samples[UNICORN_EEG_CHANNEL_COUNT];
static uint32_t sampleIndex = 0;

// ---

bool isFrameValid()
{
    return frame.buffer[0] == UNICORN_START_BYTE0
    && frame.buffer[1] == UNICORN_START_BYTE1
    && frame.buffer[frame.length - 2] == UNICORN_STOP_BYTE0
    && frame.buffer[frame.length - 1] == UNICORN_STOP_BYTE1;
}

void parseEEG()
{
    for (uint8_t channelIndex = 0; channelIndex < UNICORN_EEG_CHANNEL_COUNT; channelIndex++)
    {
        /* 3 bytes big-endian signed 24-bit */
        int32_t raw = ((int32_t)frame.buffer[3 + 3 * channelIndex] << 16)
                    | ((int32_t)frame.buffer[4 + 3 * channelIndex] << 8)
                    | ((int32_t)frame.buffer[5 + 3 * channelIndex]);

        /* two's complement for 24-bit */
        if (raw & 0x00800000)
        {
            raw |= 0xFF000000;
            raw -= 0x01000000;
        }
        samples[channelIndex] = (float)raw * UNICORN_EEG_SCALE;
    }
}

// ---

void connectUnicornEEGSource(const char *port, uint32_t timeout)
{
    if (serialOpen(&handle, port, timeout)) exit(EXIT_FAILURE);

    serialWrite(&handle,
        UNICORN_START_ACQUISITION_COMMAND,
        UNICORN_COMMAND_LENGTH
    );

    uint8_t response[3];
    int attempts = 100;
    while (attempts-- > 0)
    {
        if (serialRead(&handle, response, 3) == 3) break;
    }

    if (
        response[0] == 0x00 &&
        response[1] == 0x00 &&
        response[2] == 0x00
    ) {
        printf("Unicorn: Connected on %s\n", port);
        createSerialFrame(frameBuffer, UNICORN_PACKET_SIZE);
        return;
    }

    fprintf(stderr, "Unicorn: Unexpected start response\n");
    serialClose(&handle);
    exit(EXIT_FAILURE);
}

void resetUnicornEEGSource()
{
    resetSerialFrame(&frame);
    sampleIndex = 0;
}

void updateUnicornEEGSource()
{
    if (readSerialFrame(&handle, &frame) != READ_STATUS_READY) return;
    if (!isFrameValid())
    {
        fprintf(stderr, "Unicorn: Invalid packet\n");
        resetSerialFrame(&frame);
    }

    parseEEG();
    uint64_t timestamp = getCurrentMicrosecondTimestamp();
    in_push_signal(&tbciInputs, samples, timestamp, sampleIndex++);

    resetSerialFrame(&frame);
}

void closeUnicornEEGSource()
{
    serialWrite(&handle, UNICORN_STOP_ACQUISITION_COMMAND, UNICORN_COMMAND_LENGTH);
    serialClose(&handle);
}

bool isUnicornEEGSourceConnected() { return handle != INVALID_HANDLE_VALUE; }
uint8_t getUnicornEEGSourceChannelCount() { return UNICORN_EEG_CHANNEL_COUNT; }
uint32_t getUnicornEEGSourceSampleRate() { return UNICORN_SAMPLE_RATE; }