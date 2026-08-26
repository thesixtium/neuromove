# include "inference_logger.h"
# include <time.h>

static FILE *logFile = NULL;

void initializeInferenceLogger()
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timeString[32];
    strftime(timeString, sizeof(timeString), "%Y%m%d_%H%M%S", t);

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "tbci_inference_%s_%s_%s.csv",
        tbciContext.core_config.log_subject,
        tbciContext.core_config.log_session,
        timeString
    );

    logFile = fopen(filepath, "w");

    if (logFile) {
        fprintf(logFile, "timestamp_us,true_label,predicted_label,confidence");
        for (int i = 0; i < N_FREQS; i++)
        {
            fprintf(logFile, ",prob_%d", i);
        }
        printf("Inference Logger: logging to '%s'\n", filepath);
    }
    else
    {
        fprintf(stderr, "Failed to open inference logger file.\n");
        exit(EXIT_FAILURE);
    }
}

void logInference(TinyBCIInference inference, uint64_t timestamp, uint16_t trueLabel)
{
    if (!logFile)
    {
        printf("Failed to log inference due to failed initialization.\n");
        return;
    }

    fprintf(logFile, "\n%" PRIu64 ",%d,%d,%.6f",
        timestamp, trueLabel,
        inference.predictedLabel + 1, inference.confidence
    );
    for (int i = 0; i < N_FREQS; i++)
    {
        fprintf(logFile, "%.6f", inference.confidences[i]);
    }
    fflush(logFile);
}

void closeInferenceLogger()
{
    if (logFile)
    {
        fclose(logFile);
        logFile = NULL;
        printf("Inference Logger: file saved.\n");
    }
}
