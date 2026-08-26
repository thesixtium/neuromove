# include "lsl/inference_stream.h"
# include "lsl/helpers.h"
# include "lsl_c.h"

static lsl_outlet outlet = NULL;

void openLslInferenceOutlet()
{
    outlet = openIrregularRateLslOutlet(
        INFERENCE_STREAM_NAME, INFERENCE_STREAM_TYPE,
        INFERENCE_STREAM_CHANNEL_COUNT,
        cft_double64, INFERENCE_STREAM_SOURCE_ID
    );
}

void pushLslInference(const TinyBCIInference *inference, uint64_t microsecondTimestamp, uint16_t targetLabel)
{
    double sample[INFERENCE_STREAM_CHANNEL_COUNT];
    sample[0] = (double)inference->predictedLabel;
    sample[1] = (double)inference->confidence;
    for (int i = 0; i < N_FREQS; i++)
    {
        sample[2 + i] = (double)inference->confidences[i];
    }
    sample[2 + N_FREQS] = (double)targetLabel;
    sample[3 + N_FREQS] = (double)microsecondTimestamp;

    pushLslSample(outlet, sample);
}

void closeLslInferenceOutlet()
{
    closeLslOutlet(&outlet);
}

bool doesLslInferenceOutletHaveConsumers()
{
    return lsl_have_consumers(outlet);
}