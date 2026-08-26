# include "lsl/helpers.h"

lsl_outlet openIrregularRateLslOutlet(
    const char *streamName, const char *streamType,
    int32_t channelCount,
    lsl_channel_format_t channelFormat,
    const char* sourceId
)
{
    lsl_streaminfo outletInfo = lsl_create_streaminfo(
        streamName, streamType, channelCount,
        LSL_IRREGULAR_RATE, channelFormat, sourceId
    );
    if (outletInfo == NULL) exit(EXIT_FAILURE);

    lsl_outlet outlet = lsl_create_outlet(outletInfo, 0, 360);
    lsl_destroy_streaminfo(outletInfo);

    if (outlet == NULL)
    {
        fprintf(stderr, "Failed to open LSL outlet ");
        fprintf(stderr, "'%s'\n", streamName);
        exit(EXIT_SUCCESS);
    }
    return outlet;
}

void pushLslSample(lsl_outlet outlet, void *sample)
{
    if (outlet == NULL)
    {
        fprintf(stderr, "Error: Can't push to a null outlet\n");
        return;
    }

    int32_t pushError = lsl_push_sample_v(outlet, sample);
    if (pushError != lsl_no_error)
    {
        printf("Error pushing trigger value to LSL stream\n");
        exit(EXIT_SUCCESS);
    }
}

void closeLslOutlet(lsl_outlet *outlet)
{
    if (*outlet == NULL) return;

    lsl_destroy_outlet(*outlet);
    *outlet = NULL;
}