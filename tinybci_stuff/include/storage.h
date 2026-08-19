# pragma once
# ifndef STORAGE
# define STORAGE

# include "tbci_context.h"
# include "nodes/preprocessing/tbci_bandpass_node.h"
# include "nodes/preprocessing/tbci_notch_node.h"
# include "nodes/features/tbci_cca_node.h"
# include "nodes/decoder/tbci_cca_model.h"
# include "nodes/decoder/tbci_label_encoder_node.h"
# include "nodes/decoder/tbci_trial_averaging_node.h"

# define SIG_CAPACITY 1024
# define TRIG_CAPACITY 32
# define EPOCH_CAPACITY 8

# define WINDOW_LENGTH_MS 2000
# define WINDOW_OVERLAP_MS 1900

// ---

void allocateDynamicStorage(uint8_t, uint32_t);
void deallocateDynamicStorage();

// ---

extern size_t totalFrames;

extern float *signalStorage;
extern uint64_t *signalTimestamps;
extern uint32_t *signalIndices;

extern float *processedSignalStorage;
extern uint64_t *processedSignalTimestamps;
extern uint32_t *processedSignalIndices;

extern TBCI_Trigger *triggerStorage;

extern TBCI_Epoch *epochStorage;
extern float *epochPool;
extern TBCI_Epoch *featuresStorage;
extern float *featuresPool;
extern TBCI_Epoch *outputStorage;
extern float *outputPool;

extern TBCI_SignalBuffer signalBuffer;
extern TBCI_SignalBuffer processedSignalBuffer;

extern TBCI_TriggerQueue triggerQueue;
extern TBCI_EpochQueue epochQueue;
extern TBCI_EpochQueue featuresQueue;
extern TBCI_EpochQueue outputQueue;

extern TBCI_Input tbciInputs;
extern TBCI_Config tbciConfiguration;
extern TBCI_Context tbciContext;

// CCA constants
# define N_FREQS 4
# define N_HARMONICS 3
# define N_COMPONENTS (N_HARMONICS * 2)
extern size_t referenceSignalsCapacity;
extern float *refSignals;

// Nodes
extern TBCI_NotchNode notchNode;
extern TBCI_NotchConfig notchConfiguration;
extern TBCI_BandpassNode bandpassNode;
extern TBCI_BandpassConfig bandpassConfiguration;
extern TBCI_CCANode ccaNode;
extern TBCI_CCAConfig ccaConfiguration;
extern TBCI_CCAModel ccaModel;
extern TBCI_CCAModelConfig ccaModelConfiguration;
extern TBCI_TrialAveragingNode trialAveragingNode;
extern TBCI_TrialAveragingConfig trialAveragingConfiguration;

#endif