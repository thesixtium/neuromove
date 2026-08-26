# pragma once
# ifndef LSL_HELPERS
# define LSL_HELPERS

# include "lsl_c.h"

lsl_outlet openIrregularRateLslOutlet(
    const char *streamName, const char *streamType,
    int32_t channelCount,
    lsl_channel_format_t channelFormat,
    const char* sourceId
);
void pushLslSample(lsl_outlet outlet, void *sample);
void closeLslOutlet(lsl_outlet *outlet);

lsl_inlet connectAndOpenLslInlet(lsl_streaminfo targetStream);
void closeLslInlet(lsl_inlet *inlet);

lsl_continuous_resolver createLslResolver(const char* predicate);
void closeLslResolver(lsl_continuous_resolver *resolver);

typedef struct {
    lsl_inlet inlet;
    lsl_continuous_resolver streamResolver;
    bool isConnected;

    void *sampleBuffer;
    void (*sampleCallback)(void*);

    int32_t sampleBufferLength;
    bool bufferMemoryAllocated;
} LSLDataSource;

LSLDataSource createLSLDataSource(const char* streamResolutionPredicate);
bool tryConnectLSLDataSource(LSLDataSource *source);
bool pollLSLDataSource(LSLDataSource *source);
void closeLSLDataSource(LSLDataSource *source);

bool isLSLDataSourceConnected(LSLDataSource *source);
void setLSLDataSourceCallback(LSLDataSource *source, void (*callback)(void*));

# endif