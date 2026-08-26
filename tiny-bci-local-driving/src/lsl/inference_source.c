# include "lsl/inference_stream.h"
# include "lsl/helpers.h"

static LSLDataSource dataSource;

void initializeLslInferenceSource()
{
    dataSource = createLSLDataSource(INFERENCE_STREAM_PREDICATE);
}

bool pollLslInferenceSource(TinyBCIInference *out, uint64_t *microsecondTimestamp)
{
    if (pollLSLDataSource(&dataSource))
    {
        double* sample = (double*)dataSource.sampleBuffer;

        out->predictedLabel = (uint16_t)sample[0];
        out->confidence = (float)sample[1];
        for (int i = 0; i < N_FREQS; i++)
        {
            out->confidences[i] = (float)sample[2 + i];
        }
        out->targetLabel = (uint16_t)sample[2 + N_FREQS];
        *microsecondTimestamp = (uint64_t)sample[3 + N_FREQS];

        return true;
    }
    return false;
}

void closeLslInferenceSource() { closeLSLDataSource(&dataSource); }

bool tryConnectLslInferenceSource() { return tryConnectLSLDataSource(&dataSource); }
bool isLslInferenceSourceConnected() { return dataSource.isConnected; }