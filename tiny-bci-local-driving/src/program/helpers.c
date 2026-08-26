# include "program/helpers.h"
# include "program/constants.h"

# include "pipeline.h"
# include "microsecond_timer.h"

void awaitConnection(
    bool (*predicate)(),
    void (*updateMethod)(),
    bool (*attemptMethod)()
)
{
    MicrosecondTimer connectionAttemptTimer =
        createMicrosecondTimer(CONNECTION_ATTEMPT_INTERVAL);
    resetMicrosecondTimer(&connectionAttemptTimer);

    while(!predicate())
    {
        while (!checkMicrosecondTimer(&connectionAttemptTimer))
        {
            updateMethod();
        }
        attemptMethod();
    }
}

void printInference(TinyBCIInference inference, uint64_t timestamp)
{
    printf(
        "%" PRIu64 " | Output received: %d (%.0f%% confidence) [",
        timestamp, inference.predictedLabel, inference.confidence * 100
    );
    for (int i = 0; i < N_FREQS; i++) printf(" %.2f", inference.confidences[i]);
    printf(" ]\n");
}