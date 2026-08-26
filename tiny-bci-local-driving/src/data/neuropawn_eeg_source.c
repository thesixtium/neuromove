# include "data/neuropawn_eeg_source.h"
# include "data/serial.h"
# include "storage.h"
# include "microsecond_timer.h"

static SerialHandle handle = INVALID_HANDLE_VALUE;

static uint8_t frameBuffer[NEUROPAWN_IMU_PAYLOAD_LEN];
static SerialFrame frame;

static float eegScale;
static float samples[NEUROPAWN_EEG_CHANNEL_COUNT];
static uint32_t sampleIndex = 0;
static uint8_t expectedSampleIndex = 0;
static bool sampleIndexExpectationSet = false;

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
    int attempts = 200;  /* ~10 s at 50 ms per read */

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

int findStartByte()
{
    return seekSerialByte(&handle, NEUROPAWN_START_BYTE);
}

ReadStatus readFrame()
{
    if (frame.cursor == 0)
    {
        if (findStartByte()) return READ_STATUS_INVALID;
        frame.buffer[0] = NEUROPAWN_START_BYTE;
        frame.cursor = 1;
    }

    ReadStatus readStatus = readSerialFrame(&handle, &frame);
    if (readStatus == READ_STATUS_READY)
    {
        if (frame.buffer[frame.length - 1] != NEUROPAWN_END_BYTE)
        {
            fprintf(stderr, "neuropawn: dropped packet due to misaligned frame\n");
            serialRead(&handle, frame.buffer, 1); // offset frame
            resetSerialFrame(&frame);
            return READ_STATUS_INVALID;
        }
    }
    return readStatus;
}

EXGStatus validateEXGFrame()
{
    if (
        frame.buffer[0] != NEUROPAWN_START_BYTE || 
        frame.buffer[frame.length - 1] != NEUROPAWN_END_BYTE
    ) {
        fprintf(stderr, "neuropawn: payload invalid, frame is misaligned\n");
        return EXG_STATUS_MISALIGNED;
    }

    uint8_t frameIndex = frame.buffer[1];
    if (sampleIndexExpectationSet && frameIndex != expectedSampleIndex)
    {
        fprintf(stderr,
            "neuropawn: payload index %u doesn't "
            "match expected index of %u\n",
            frameIndex, expectedSampleIndex
        );
        expectedSampleIndex = frameIndex + 1;
        return EXG_STATUS_UNEXPECTED_SAMPLE_INDEX;
    }
    expectedSampleIndex = frameIndex + 1;
    sampleIndexExpectationSet = true;
    return EXG_STATUS_VALID;
}

void parseEXG()
{
    for (size_t channelIndex = 0; channelIndex < NEUROPAWN_EEG_CHANNEL_COUNT; channelIndex++)
    {
        int16_t raw = (int16_t)(((uint16_t)frame.buffer[1 + 2 * channelIndex] << 8) |
                                 (uint16_t)frame.buffer[2 + 2 * channelIndex]);
        samples[channelIndex] = (float)raw * eegScale;
    }
}

// ---

int awaitFrame()
{
    uint16_t scanAttempts = 400;
    resetSerialFrame(&frame);
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
    
    if (
        frame.buffer[1 + 2 * channelIndex] ||
        frame.buffer[2 + 2 * channelIndex]
    ) return EXIT_SUCCESS;
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

    uint8_t payloadLength = boardType == NEUROPAWN_BOARD_IMU
        ? NEUROPAWN_IMU_PAYLOAD_LEN
        : NEUROPAWN_EEG_PAYLOAD_LEN;
    const char * typeString = (boardType == NEUROPAWN_BOARD_IMU) ? "IMU" : "non-IMU";
    printf("neuropawn: connected on %s (%s board)\n", port, typeString);

    frame = createSerialFrame(frameBuffer, payloadLength);

    printf("neuropawn: configuring channels (gain %u)...\n", config.gain);
    configureChannels(config);
}

void resetNeuropawnEEGSource()
{
    resetSerialFrame(&frame);
    serialFlush(&handle);
}

void updateNeuropawnEEGSource()
{
    if (readFrame() != READ_STATUS_READY) return;
    if (validateEXGFrame() != EXG_STATUS_VALID)
    {
        resetSerialFrame(&frame);
        return;
    }

    parseEXG();
    uint64_t timestamp = getCurrentMicrosecondTimestamp();
    in_push_signal(&tbciInputs, samples, timestamp, sampleIndex++);

    resetSerialFrame(&frame);
}

void closeNeuropawnEEGSource() { serialClose(&handle); }

bool isNeuropawnEEGSourceConnected() { return handle != INVALID_HANDLE_VALUE; }
uint8_t getNeuropawnEEGSourceChannelCount() { return NEUROPAWN_EEG_CHANNEL_COUNT; }
uint32_t getNeuropawnEEGSourceSampleRate() { return NEUROPAWN_SAMPLE_RATE; }