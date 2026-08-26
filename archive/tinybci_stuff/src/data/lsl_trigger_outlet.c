# include "data/lsl_trigger_outlet.h"
# include "lsl_c.h"

static lsl_outlet outlet = NULL;

void openLslTriggerOutlet(const char *streamName)
{
    lsl_streaminfo outletInfo = lsl_create_streaminfo(
        streamName,
        TRIGGER_STREAM_TYPE,
        1, LSL_IRREGULAR_RATE,
        cft_int16,
        TRIGGER_STREAM_SOURCE_ID
    );
    if (outletInfo == NULL) exit(EXIT_FAILURE);

    outlet = lsl_create_outlet(outletInfo, 0, 360);
    lsl_destroy_streaminfo(outletInfo);

    if (outlet == NULL)
    {
        printf("Failed to open LSL trigger outlet\n");
        exit(EXIT_SUCCESS);
    }
}

void pushLslTrigger(uint16_t value)
{
    if (outlet == NULL)
    {
        printf("Attempting to start LSL trigger stream\n");
        openLslTriggerOutlet(TRIGGER_STREAM_NAME_DEFAULT);
    }

    int16_t sample[1] = {value};
    int32_t pushError = lsl_push_sample_s(outlet, sample);

    if (pushError != lsl_no_error)
    {
        printf("Error pushing trigger value to LSL stream\n");
        exit(EXIT_SUCCESS);
    }
}

void closeLslTriggerOutlet()
{
    if (outlet != NULL)
    {
        lsl_destroy_outlet(outlet);
        outlet = NULL;
    }
}