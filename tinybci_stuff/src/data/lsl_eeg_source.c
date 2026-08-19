# include "data/lsl_eeg_source.h"
# include "pipeline.h"
# include "lsl_c.h"

# ifndef USE_LSL_TIMESTAMPS
#   include "microsecond_timer.h"
# endif


static uint32_t sampleRate = 0;
static uint8_t channelCount = 0;

static float* samples = NULL;
static uint32_t sampleIndex = 0;

static lsl_inlet inlet = NULL;
static bool isConnected = false;

// ---

void connectLslEEGSource()
{
    lsl_streaminfo scanResults[2];
    int resultCount = lsl_resolve_bypred(scanResults, 2, LSL_EEG_PREDICATE, 1, LSL_SCAN_TIMEOUT);

    if (resultCount < 1)
    {
        printf("Failed to locate LSL EEG Source\n");
        exit(EXIT_SUCCESS);
    }
    else if (resultCount > 1)
    {
        printf("Cannot choose between 2 or more EEG streams\n");
        exit(EXIT_SUCCESS);
    }

    lsl_streaminfo targetStream = scanResults[0];
    channelCount = (uint8_t)lsl_get_channel_count(targetStream);
    sampleRate = (uint32_t)lsl_get_nominal_srate(targetStream);

    inlet = lsl_create_inlet(targetStream, 360, LSL_NO_PREFERENCE, 1);
    lsl_destroy_streaminfo(targetStream);

    if (inlet == NULL)
    {
        printf("Failed to create LSL inlet\n");
        exit(EXIT_SUCCESS);
    }

    int32_t openError = 0;
    lsl_open_stream(inlet, LSL_CONNECT_TIMEOUT, &openError);

    if (openError != lsl_no_error)
    {
        lsl_destroy_inlet(inlet);
        printf("Failed to connect to LSL EEG stream\n");
        exit(EXIT_SUCCESS);
    }
    isConnected = true;

    int32_t infoError = 0;
    lsl_streaminfo inletInfo = lsl_get_fullinfo(inlet, LSL_SCAN_TIMEOUT, &infoError);
    if (infoError == lsl_no_error)
    {
        const char* streamName = lsl_get_name(inletInfo);
        printf("Connected to LSL EEG stream \"%s\"", streamName);
        printf(" with %d channels sampling at %d Hz\n", channelCount, sampleRate);
        printf("---\n");
    }
    lsl_destroy_streaminfo(inletInfo);

    samples = malloc(channelCount * sizeof(float));
}

// ---

void updateLslEEGSource()
{
    if (!isConnected)
    {
        printf("Attempting to connect to LSL EEG stream\n");
        connectLslEEGSource();
    }

    int32_t pullError = 0;
    double lslTimestamp = lsl_pull_sample_f(inlet, samples, channelCount, 0.0, &pullError);

    if (pullError != lsl_no_error)
    {
        disconnectLslEEGSource();
        printf("Pull error %u in LSL EEG source\n", pullError);
        exit(EXIT_SUCCESS);
    }

    if (lslTimestamp > 0.0)
    {
# ifdef USE_LSL_TIMESTAMPS
        uint64_t microsecondTimestamp = (uint64_t)(lslTimestamp * 1000000);
# else
        uint64_t microsecondTimestamp = getCurrentMicrosecondTimestamp();
# endif
        in_push_signal(&tbciInputs, samples, microsecondTimestamp, sampleIndex++);
    }
}

void disconnectLslEEGSource()
{
    if (isConnected) free(samples);
    if (inlet != NULL)
    {
        lsl_close_stream(inlet);
        lsl_destroy_inlet(inlet);
        inlet = NULL;
    }
    isConnected = false;
}

// ---

uint8_t getLslEEGSourceChannelCount() { return channelCount; }
uint32_t getLslEEGSourceSampleRate() { return sampleRate; }