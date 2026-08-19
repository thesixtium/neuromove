# pragma once
# include "pipeline.h"

void initializeInferenceLogger();
void logInference(TinyBCIInference inference, uint64_t timestamp, uint16_t trueLabel);
void closeInferenceLogger();