# pragma once
# include "storage.h"

int initializeTinyBCIPipeline(const float *, uint8_t, uint32_t);
void cleanUpTinyBCIPipeline();

int startTinyBCIPipeline();
int startTinyBCIPipelineInState(TBCI_State);
int updateTinyBCIPipeline();
int stopTinyBCIPipeline();

typedef struct {
    int16_t predictedLabel;
    uint16_t targetLabel;
    float confidence;
    float confidences[N_FREQS];
} TinyBCIInference;

bool tryGetTinyBCIInference(TinyBCIInference *);