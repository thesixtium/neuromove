# include "lsl/trigger_stream.h"
# include "lsl/helpers.h"
# include "lsl_c.h"

static lsl_outlet outlet = NULL;

void openLslTriggerOutlet()
{
    outlet = openIrregularRateLslOutlet(
        TRIGGER_STREAM_NAME, TRIGGER_STREAM_TYPE,
        1, cft_int16, TRIGGER_STREAM_SOURCE_ID
    );
}

void pushLslTrigger(uint16_t value)
{
    int16_t sample[1] = {value};
    pushLslSample(outlet, sample);
}

void closeLslTriggerOutlet() {
    closeLslOutlet(&outlet);
}

bool doesLslTriggerOutletHaveConsumers()
{
    return lsl_have_consumers(outlet);
}