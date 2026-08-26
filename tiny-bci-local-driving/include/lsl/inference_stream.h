# pragma once
# ifndef LSL_INFERENCE_OUTLET
# define LSL_INFERENCE_OUTLET

# include "pipeline.h"

/* 
--- Inference Packet Format ---
[0]: predicted label
[1]: prediction confidence
[2 .. 1 + N_FREQS]: confidences
[2 + N_FREQS]: targetLabel
[3 + N_FREQS]: microsecond timestamp
*/
# define INFERENCE_STREAM_CHANNEL_COUNT (4 + N_FREQS)
# define INFERENCE_STREAM_NAME "Tiny_BCI_Inferences"
# define INFERENCE_STREAM_TYPE "Inferences"
# define INFERENCE_STREAM_SOURCE_ID "tiny_bci_ssvep_experiment_inferences"

# define INFERENCE_STREAM_PREDICATE "type='" INFERENCE_STREAM_TYPE "'"

void openLslInferenceOutlet();
void pushLslInference(const TinyBCIInference *inference, uint64_t microsecondTimestamp, uint16_t targetLabel);
void closeLslInferenceOutlet();

bool doesLslInferenceOutletHaveConsumers();

void initializeLslInferenceSource();
bool pollLslInferenceSource(TinyBCIInference *out, uint64_t *microsecondTimestamp);
void closeLslInferenceSource();

bool tryConnectLslInferenceSource();
bool isLslInferenceSourceConnected();

# endif