# include "data/neuropawn_eeg_source.h"
# include "data/serial.h"
# include "microsecond_timer.h"

static SerialHandle handle = INVALID_HANDLE_VALUE;

static uint8_t payload[NEUROPAWN_IMU_PAYLOAD_LEN];
static uint8_t payloadLength;
static uint8_t payloadCursor = 0;

static float eegScale;
static float samples[NEUROPAWN_EEG_CHANNEL_COUNT];
static uint32_t sampleIndex = 0;
static uint8_t expectedSampleIndex = 0;
static bool sampleIndexExpectationSet = false;

typedef enum {
    READ_STATUS_READY,
    READ_STATUS_PENDING,
    READ_STATUS_INVALID
} ReadStatus;

typedef enum {
    EXG_STATUS_VALID,
    EXG_STATUS_MISALIGNED,
    EXG_STATUS_UNEXPECTED_SAMPLE_INDEX
} EXGStatus;

// ---

NeuroPawnBoardType scanFrameSize(const uint8_t *buffer, size_t bufferLength)
{
    for (size_t i = 0; i < bufferLength; i++) {
        if (buffer[i] != NEUROPAWN_START_BYTE)
            continue;

        /* non-IMU: stride 21 */
        if (i + 42 < bufferLength &&
            buffer[i + 20] == NEUROPAWN_END_BYTE   &&
            buffer[i + 21] == NEUROPAWN_START_BYTE &&
            buffer[i + 41] == NEUROPAWN_END_BYTE   &&
            buffer[i + 42] == NEUROPAWN_START_BYTE)
            return NEUROPAWN_BOARD_EEG;

        /* IMU: stride 57 */
        if (i + 114 < bufferLength &&
            buffer[i + 56]  == NEUROPAWN_END_BYTE   &&
            buffer[i + 57]  == NEUROPAWN_START_BYTE &&
            buffer[i + 113] == NEUROPAWN_END_BYTE   &&
            buffer[i + 114] == NEUROPAWN_START_BYTE)
            return NEUROPAWN_BOARD_IMU;
    }
    return NEUROPAWN_BOARD_UNKNOWN;
}

NeuroPawnBoardType detectBoardType()
{
    static uint8_t buffer[8192];
    size_t scanLength = 0;
    int attempts = 10;  /* ~10 s at 50 ms per read */

    serialFlush(&handle);

    while (attempts-- > 0) {
        if (scanLength >= sizeof(buffer)) {
            /* keep only the tail so detection stays bounded */
            memmove(buffer, buffer + scanLength - 1024, 1024);
            scanLength = 1024;
        }
        int readCount = serialRead(&handle, buffer + scanLength, sizeof(buffer) - scanLength);
        if (readCount > 0) {
            scanLength += (size_t)readCount;
            NeuroPawnBoardType type = scanFrameSize(buffer, scanLength);
            if (type != NEUROPAWN_BOARD_UNKNOWN)
                return type;
        }
    }
    return NEUROPAWN_BOARD_UNKNOWN;
}

// ---

void resetPayload()
{
    for (int i = 0; i < payloadLength; i++) payload[i] = 0;
    payloadCursor = 0;
}

int findStartByte()
{
    uint16_t scanAttempts = 200;
    while (payload[0] != NEUROPAWN_START_BYTE)
    {
        serialRead(&handle, payload, 1);
        if (scanAttempts-- <= 0)
        {
            fprintf(stderr, "neuropawn: failed to locate start byte\n");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

ReadStatus readFrame()
{
    if (payloadCursor == 0)
    {
        if (findStartByte()) return READ_STATUS_INVALID;
        payloadCursor = 1;
    }

    int readCount = serialRead(&handle, payload + payloadCursor, payloadLength - payloadCursor);
    if (readCount > 0) payloadCursor += readCount;

    if (payloadCursor < payloadLength) return READ_STATUS_PENDING;
    else
    {
        if (payload[payloadLength - 1] != NEUROPAWN_END_BYTE)
        {
            fprintf(stderr, "neuropawn: dropped packet due to misaligned frame\n");
            serialRead(&handle, payload, 1); // offset frame
            resetPayload();
            return READ_STATUS_INVALID;
        }
        return READ_STATUS_READY;
    }
}

EXGStatus validateEXGFrame()
{
    if (payload[0] != NEUROPAWN_START_BYTE || payload[payloadLength - 1] != NEUROPAWN_END_BYTE)
    {
        fprintf(stderr, "neuropawn: payload invalid, frame is misaligned\n");
        return EXG_STATUS_MISALIGNED;
    }
    if (sampleIndexExpectationSet && payload[1] != expectedSampleIndex)
    {
        fprintf(stderr, "neuropawn: payload index %u doesn't match expected index of %u\n", payload[1], expectedSampleIndex);
        expectedSampleIndex = payload[1] + 1;
        return EXG_STATUS_UNEXPECTED_SAMPLE_INDEX;
    }
    expectedSampleIndex = payload[1] + 1;
    sampleIndexExpectationSet = true;
    return EXG_STATUS_VALID;
}

void parseEXG()
{
    for (size_t channelIndex = 0; channelIndex < NEUROPAWN_EEG_CHANNEL_COUNT; channelIndex++)
    {
        int16_t raw = (int16_t)(((uint16_t)payload[1 + 2 * channelIndex] << 8) |
                                 (uint16_t)payload[2 + 2 * channelIndex]);
        samples[channelIndex] = (float)raw * eegScale;
    }
}

// ---

int awaitFrame()
{
    uint16_t scanAttempts = 400;
    resetPayload();
    serialFlush(&handle);
    while (readFrame() != READ_STATUS_READY)
    {
        if (scanAttempts-- < 0) return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int awaitEXGChannelData(uint8_t channelIndex, bool requireValues)
{
    if (awaitSerialData(&handle)) return EXIT_FAILURE;
    if (awaitFrame()) return EXIT_FAILURE;
    if (!requireValues) return EXIT_SUCCESS;
    
    if (payload[1 + 2 * channelIndex] || payload[2 + 2 * channelIndex]) return EXIT_SUCCESS;
    return EXIT_FAILURE;
}

void sendCommand(const char *command)
{
    serialWrite(&handle, (uint8_t *)command, strlen(command));
}

void configureChannel(const char* cmd, uint8_t channelIndex, bool expectNonZeroSamples)
{
    sendCommand(cmd);
    sleepMilliseconds(NEUROPAWN_CMD_PAUSE_MS);
    while (awaitEXGChannelData(channelIndex, expectNonZeroSamples))
    {
        fprintf(stderr, "neuropawn: failed to configure channel, retrying\n");
        sendCommand(cmd);
    }
}

// ---

void configureChannels(NeuropawnConfiguration config)
{
    char cmd[32];

    for (uint8_t channelIndex = 0; channelIndex < NEUROPAWN_EEG_CHANNEL_COUNT; channelIndex++)
    {
        int channelLabel = (int)channelIndex + 1;
        bool channelEnabled = config.activateChannel[channelIndex];
        
        /* per-channel enable / disable */
        if (channelEnabled)
        {
            snprintf(cmd, sizeof cmd, "chon_%d_%u", channelLabel, config.gain);
            printf("neuropawn: enabling channel %d\n", channelLabel);
        }
        else
        {
            snprintf(cmd, sizeof cmd, "choff_%d", channelLabel);
            printf("neuropawn: disabling channel %d\n", channelLabel);
        }
        configureChannel(cmd, channelIndex, channelEnabled);
        
        /* optional right-leg-drive */
        if (config.activateRightLegDrive[channelIndex]) {
            snprintf(cmd, sizeof cmd, "rldadd_%d", channelLabel);
            printf("neuropawn: enabling right leg drive for channel %d\n", channelLabel);
            configureChannel(cmd, channelIndex, channelEnabled);
        }
    }
}

void connectNeuropawnEEGSource(const char *port, NeuropawnConfiguration config)
{
    printf("neuropawn: attempting to connect on %s\n", port);

    if (serialOpen(&handle, port, config.timeout)) exit(EXIT_SUCCESS);
    eegScale = (4.0f / 32767.0f / config.gain * 1000000.0f);

    sleepMilliseconds(NEUROPAWN_CMD_PAUSE_MS);
    awaitSerialData(&handle);

    printf("neuropawn: detecting board type from incoming packets...\n");
    if (findStartByte())
    {
        fprintf(stderr, "neuropawn: failed to locate start of frame from which to scan\n");
        exit(EXIT_FAILURE);
    }

    NeuroPawnBoardType boardType = detectBoardType();
    if (boardType == NEUROPAWN_BOARD_UNKNOWN)
    {
        fprintf(stderr, "neuropawn: detection failed - no valid packets.\n");
        serialClose(&handle);
        exit(EXIT_SUCCESS);
    }

    payloadLength = boardType == NEUROPAWN_BOARD_IMU
        ? NEUROPAWN_IMU_PAYLOAD_LEN
        : NEUROPAWN_EEG_PAYLOAD_LEN;
    const char * typeString = (boardType == NEUROPAWN_BOARD_IMU) ? "IMU" : "non-IMU";
    printf("neuropawn: connected on %s (%s board)\n", port, typeString);

    printf("neuropawn: configuring channels (gain %u)...\n", config.gain);
    configureChannels(config);
}

void resetNeuropawnEEGSource()
{
    resetPayload();
    serialFlush(&handle);
}

void updateNeuropawnEEGSource()
{
    if (readFrame() != READ_STATUS_READY) return;
    if (validateEXGFrame() != EXG_STATUS_VALID)
    {
        resetPayload();
        return;
    }

    parseEXG();
    uint64_t timestamp = getCurrentMicrosecondTimestamp();
    in_push_signal(&tbciInputs, samples, timestamp, sampleIndex++);

    resetPayload();
}

void disconnectNeuropawnEEGSource() { serialClose(&handle); }