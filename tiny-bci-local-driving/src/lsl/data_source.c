# include "lsl/helpers.h"
# include "lsl/constants.h"

void connectLSLDataSource(LSLDataSource *source, lsl_streaminfo targetStream)
{
    source->inlet = connectAndOpenLslInlet(targetStream);
    source->isConnected = true;

    int32_t bufferLength = lsl_get_sample_bytes(targetStream);
    source->sampleBuffer = malloc(bufferLength);
    source->sampleBufferLength = bufferLength;
    source->bufferMemoryAllocated = true;
}

bool tryConnectLSLDataSource(LSLDataSource *source)
{
    if (source->streamResolver == NULL)
    {
        fprintf(stderr, "Error: Can't connect uninitialized data source\n");
        return false;
    }
    if (source->isConnected) return true;

    lsl_streaminfo scanResult;
    if (lsl_resolver_results(source->streamResolver, &scanResult, 1))
    {
        connectLSLDataSource(source, scanResult);
        lsl_destroy_streaminfo(scanResult);
        return true;
    }
    return false;
}

LSLDataSource createLSLDataSource(const char* streamResolutionPredicate)
{
    LSLDataSource dataSource = (LSLDataSource)
    {
        .inlet = NULL,
        .streamResolver = createLslResolver(streamResolutionPredicate),
        .isConnected = false,

        .sampleBuffer = NULL,
        .sampleCallback = NULL,

        .sampleBufferLength = 0,
        .bufferMemoryAllocated = false
    };
    tryConnectLSLDataSource(&dataSource);

    return dataSource;
}

bool pollLSLDataSource(LSLDataSource *source)
{
    if (source == NULL)
    {
        fprintf(stderr, "Error: Can't poll a null data source\n");
        exit(EXIT_FAILURE);
    }
    if (!source->isConnected)
    {
        if (source->streamResolver != NULL)
        {
            if (!tryConnectLSLDataSource(source)) return false;
        }
        else
        {
            fprintf(stderr, "Error: Can't poll a disconnected data source\n");
            return false;
        }
    }

    int32_t pullError = 0;
    double lslTimestamp = lsl_pull_sample_v(
        source->inlet, source->sampleBuffer,
        source->sampleBufferLength, 0.0, &pullError
    );

    if (pullError != lsl_no_error)
    {
        fprintf(
            stderr, "Pull error %u in LSL data source: %s\n",
            pullError, lsl_last_error()
        );
        closeLSLDataSource(source);
        return false;
    }

    bool samplePulled = lslTimestamp > 0.0;
    if (samplePulled && source->sampleCallback != NULL)
    {
        source->sampleCallback(source->sampleBuffer);
    }
    return samplePulled;
}

void closeLSLDataSource(LSLDataSource *source)
{
    if (source == NULL) return;
    if (source->bufferMemoryAllocated)
    {
        free(source->sampleBuffer);
        source->sampleBuffer = NULL;
        source->bufferMemoryAllocated = false;
        source->sampleBufferLength = 0;
    }
    closeLslResolver(&(source->streamResolver));
    closeLslInlet(&(source->inlet));
    source->isConnected = false;
}

bool isLSLDataSourceConnected(LSLDataSource *source)
{
    return source->isConnected && source->inlet != NULL;
}

void setLSLDataSourceCallback(LSLDataSource *source, void (*callback)(void*))
{
    source->sampleCallback = callback;
}