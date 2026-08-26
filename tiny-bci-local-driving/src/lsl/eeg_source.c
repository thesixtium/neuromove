# include "lsl/eeg_source.h"
# include "lsl/helpers.h"
# include "lsl/constants.h"
# include "pipeline.h"
# include "microsecond_timer.h"
# include "lsl_c.h"

static const float ConnectionAttemptInterval = 1.0;

static uint32_t sampleRate = 0;
static uint8_t channelCount = 0;

static LSLDataSource dataSource;
static uint32_t sampleIndex = 0;

// ---

void pushSample(void *sampleBuffer)
{
# ifdef USE_LSL_TIMESTAMPS
    uint64_t microsecondTimestamp = (uint64_t)(lslTimestamp * 1000000);
# else
    uint64_t microsecondTimestamp = getCurrentMicrosecondTimestamp();
# endif

    in_push_signal(
        &tbciInputs, (float*)sampleBuffer,
        microsecondTimestamp, sampleIndex++
    );
}

void connectLslEEGSource()
{
    dataSource = createLSLDataSource(EEG_STREAM_PREDICATE);
    setLSLDataSourceCallback(&dataSource, pushSample);

    MicrosecondTimer connectionAttemptTimer = createMicrosecondTimer(ConnectionAttemptInterval);
    resetMicrosecondTimer(&connectionAttemptTimer);
    while (!dataSource.isConnected)
    {
        while (checkMicrosecondTimer(&connectionAttemptTimer));
        tryConnectLSLDataSource(&dataSource);
    }

    int32_t infoError = 0;
    lsl_streaminfo inletInfo = lsl_get_fullinfo(
        dataSource.inlet, LSL_INFO_TIMEOUT, &infoError
    );

    if (infoError != lsl_no_error)
    {
        fprintf(stderr, "Failed to get EEG stream info");
        closeLslEEGSource();
        exit(EXIT_SUCCESS);
    }

    channelCount = (uint8_t)lsl_get_channel_count(inletInfo);
    sampleRate = (uint32_t)lsl_get_nominal_srate(inletInfo);

    const char* streamName = lsl_get_name(inletInfo);
    printf(
        "Connected to LSL EEG stream '%s' with "
        "%d channels sampling at %d Hz\n"
        , streamName, channelCount, sampleRate
    );
    lsl_destroy_streaminfo(inletInfo);
}

void updateLslEEGSource() { pollLSLDataSource(&dataSource); }
void closeLslEEGSource() { closeLSLDataSource(&dataSource); }

bool isLslEEGSourceConnected() { return dataSource.isConnected; }

// ---

uint8_t getLslEEGSourceChannelCount() { return channelCount; }
uint32_t getLslEEGSourceSampleRate() { return sampleRate; }