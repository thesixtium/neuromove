# include "storage.h"

uint64_t signalTimestampsArray[SIG_CAPACITY];
uint32_t signalIndicesArray[SIG_CAPACITY];

uint64_t processedSignalTimestampsArray[SIG_CAPACITY];
uint32_t processedSignalIndicesArray[SIG_CAPACITY];

TBCI_Trigger triggerStorageArray[TRIG_CAPACITY];

TBCI_Epoch epochStorageArray[EPOCH_CAPACITY];
TBCI_Epoch featuresStorageArray[EPOCH_CAPACITY];
TBCI_Epoch outputStorageArray[EPOCH_CAPACITY];


size_t totalFrames = 0;

float *signalStorage = NULL;
uint64_t *signalTimestamps = signalTimestampsArray;
uint32_t *signalIndices = signalIndicesArray;

float *processedSignalStorage = NULL;
uint64_t *processedSignalTimestamps = processedSignalTimestampsArray;
uint32_t *processedSignalIndices = processedSignalIndicesArray;

TBCI_Trigger *triggerStorage = triggerStorageArray;

TBCI_Epoch *epochStorage = epochStorageArray;
float *epochPool = NULL;
TBCI_Epoch *featuresStorage = featuresStorageArray;
float *featuresPool = NULL;
TBCI_Epoch *outputStorage = outputStorageArray;
float *outputPool = NULL;

TBCI_SignalBuffer signalBuffer;
TBCI_SignalBuffer processedSignalBuffer;

TBCI_TriggerQueue triggerQueue;
TBCI_EpochQueue epochQueue;
TBCI_EpochQueue featuresQueue;
TBCI_EpochQueue outputQueue;

TBCI_Input tbciInputs;
TBCI_Config tbciConfiguration;
TBCI_Context tbciContext;

size_t referenceSignalsCapacity = 0;
float *refSignals = NULL;


TBCI_NotchNode notchNode;
TBCI_NotchConfig notchConfiguration;
TBCI_BandpassNode bandpassNode;
TBCI_BandpassConfig bandpassConfiguration;
TBCI_CCANode ccaNode;
TBCI_CCAConfig ccaConfiguration;
TBCI_CCAModel ccaModel;
TBCI_CCAModelConfig ccaModelConfiguration;
TBCI_TrialAveragingNode trialAveragingNode;
TBCI_TrialAveragingConfig trialAveragingConfiguration;

// ---

void allocateDynamicStorage(uint8_t channelCount, uint32_t sampleRate)
{
    size_t signalStorageSize = SIG_CAPACITY * channelCount * sizeof(float);
    signalStorage = malloc(signalStorageSize);
    processedSignalStorage = malloc(signalStorageSize);

    totalFrames = (size_t)(sampleRate * WINDOW_LENGTH_MS / 1000);
    size_t epochPoolCapacity = EPOCH_CAPACITY * totalFrames * channelCount;
    size_t epochPoolSize = epochPoolCapacity * sizeof(float);
    epochPool = malloc(epochPoolSize);
    featuresPool = malloc(epochPoolSize);
    outputPool = malloc(epochPoolSize);

    referenceSignalsCapacity = N_FREQS * N_COMPONENTS * totalFrames;
    refSignals = malloc(referenceSignalsCapacity * sizeof(float));
}

void deallocateDynamicStorage()
{
    free(signalStorage);
    free(processedSignalStorage);
    free(epochPool);
    free(featuresPool);
    free(outputPool);
    free(refSignals);

    totalFrames = 0;
    referenceSignalsCapacity = 0;
}