# include "lsl/trigger_stream.h"
# include "lsl/helpers.h"

static LSLDataSource dataSource;
static TriggerCallback triggerCallback = NULL;

void passTrigger(void *sampleBuffer)
{
    int16_t sample = *(int16_t*)(dataSource.sampleBuffer);
    triggerCallback(sample);
}

void initializeLslTriggerSource(TriggerCallback callback)
{
    triggerCallback = callback;

    dataSource = createLSLDataSource(TRIGGER_STREAM_PREDICATE);
    setLSLDataSourceCallback(&dataSource, passTrigger);
}

void updateLslTriggerSource() { pollLSLDataSource(&dataSource); }
void closeLslTriggerSource() { closeLSLDataSource(&dataSource); }

bool tryConnectLslTriggerSource() { return tryConnectLSLDataSource(&dataSource); }
bool isLslTriggerSourceConnected() { return dataSource.isConnected; }