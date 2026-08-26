# include "pipeline.h"
# include "data/trigger_source.h"

void initializeTinyBCIPipelineStorage(uint8_t, uint32_t);
void setTinyBCIPipelineConfiguration(uint8_t, float);
void addCCANodesToTinyBCIPipeline(const float *);

int reportStatus(TBCI_Status status, const char *actionLabel)
{
    if (status)
    {
        fprintf(stderr, "Failed to %s Tiny BCI Pipeline | code: %d\n", actionLabel, status);
    }
    return status;
}

// ---

int initializeTinyBCIPipeline(const float *frequencies, uint8_t channelCount, uint32_t sampleRate)
{
    initializeTinyBCIPipelineStorage(channelCount, sampleRate);
    setTinyBCIPipelineConfiguration(channelCount, sampleRate * 1.0f);
    addCCANodesToTinyBCIPipeline(frequencies);

    TBCI_Status initializationStatus = tbci_context_init(
        &tbciContext, &tbciConfiguration,
        &tbciInputs, &processedSignalBuffer,
        &epochQueue, &featuresQueue, &outputQueue
    );
    tbciInputs.signal = &signalBuffer;
    return reportStatus(initializationStatus, "initialize");
}

void initializeTinyBCIPipelineStorage(uint8_t channelCount, uint32_t sampleRate)
{
    allocateDynamicStorage(channelCount, sampleRate);

    sb_init(&signalBuffer, signalStorage, signalTimestamps, signalIndices, SIG_CAPACITY, channelCount);
    sb_init(&processedSignalBuffer, processedSignalStorage, processedSignalTimestamps, processedSignalIndices, SIG_CAPACITY, channelCount);
    tq_init(&triggerQueue, triggerStorage, TRIG_CAPACITY);
    eq_init(&epochQueue, epochStorage, EPOCH_CAPACITY, totalFrames);
    eq_configure(&epochQueue, epochPool, channelCount);
    eq_init(&featuresQueue, featuresStorage, EPOCH_CAPACITY, totalFrames);
    eq_configure(&featuresQueue, featuresPool, channelCount);
    eq_init(&outputQueue, outputStorage, EPOCH_CAPACITY, totalFrames);
    eq_configure(&outputQueue, outputPool, channelCount);

    tbciInputs.signal = &signalBuffer;
    tbciInputs.triggers = &triggerQueue;
    tbciInputs.n_channels = channelCount;
}

void setTinyBCIPipelineConfiguration(uint8_t channelCount, float sampleRate)
{
    tbciConfiguration.paradigm = TBCI_PARADIGM_SSVEP;
    tbciConfiguration.nominal_srate = sampleRate;
    tbciConfiguration.target_srate = sampleRate;
    tbciConfiguration.n_channels = channelCount;
    tbciConfiguration.window_length_ms = WINDOW_LENGTH_MS;
    tbciConfiguration.mode = SEG_MODE_SLIDING;
    tbciConfiguration.pre_stimulus_ms = 0;
    tbciConfiguration.post_stimulus_ms = WINDOW_LENGTH_MS;
    tbciConfiguration.overlap_ms = WINDOW_OVERLAP_MS;
    tbciConfiguration.trial_end_code = TRIAL_END_CODE;
    tbciConfiguration.use_preprocessing = true;
    tbciConfiguration.use_feature_extraction = true;
    tbciConfiguration.use_decoder = true;
    tbciConfiguration.log_enabled = true; /* set true to enable CSV logging */
    tbciConfiguration.log_processed = false; /* set true to enable logging of preprocessed data */
    tbciConfiguration.log_subject[0] = '\0';
    tbciConfiguration.log_session[0] = '\0';
}

void addCCANodesToTinyBCIPipeline(const float *frequencies)
{
    /* register notch & bandpass node in preprocessing group */
    notchConfiguration.freq_hz = 60.0f;
    notchConfiguration.q_factor = 10.0f;
    notchConfiguration.n_harmonics = 2;
    notch_init(&notchNode, &notchConfiguration);

    bp_configure(&bandpassConfiguration,2.0f, 45.0f, 3);
    bp_init(&bandpassNode, &bandpassConfiguration);

    /* Register CCA node and model */
    ccaConfiguration.n_freqs = N_FREQS;
    ccaConfiguration.n_harmonics = N_HARMONICS;
    for (uint16_t i = 0; i < N_FREQS; i++)
    {
        ccaConfiguration.freqs[i] = frequencies[i];
    }

    cca_init(&ccaNode, &ccaConfiguration, refSignals, referenceSignalsCapacity);

    ccaModelConfiguration.temperature = 0.1f;
    ccaModelConfiguration.n_freqs = N_FREQS;
    cca_model_init(&ccaModel, &ccaModelConfiguration);

    trialAveragingConfiguration.n_reps = 3;
    ta_init(&trialAveragingNode, &trialAveragingConfiguration);

    group_add_node(&tbciContext.preprocessing.group, (TBCI_Node *)&notchNode);
    group_add_node(&tbciContext.preprocessing.group, (TBCI_Node *)&bandpassNode);
    group_add_node(&tbciContext.features.group, (TBCI_Node *)&ccaNode);
    group_add_node(&tbciContext.decoder.group, (TBCI_Node *)&ccaModel);
    group_add_node(&tbciContext.decoder.group, (TBCI_Node *)&trialAveragingNode);
}

// ---

void cleanUpTinyBCIPipeline()
{
    stopTinyBCIPipeline();
    deallocateDynamicStorage();
}

// ---

int startTinyBCIPipeline()
{
    TBCI_Status status = tbci_context_start(&tbciContext, TBCI_STATE_INFERENCE);
    return reportStatus(status, "start");
}
int startTinyBCIPipelineInState(TBCI_State initialState)
{
    TBCI_Status status = tbci_context_start(&tbciContext, initialState);
    return reportStatus(status, "start");
}

int updateTinyBCIPipeline()
{
    TBCI_Status status = tbci_context_tick(&tbciContext);
    return reportStatus(status, "update");
}

int stopTinyBCIPipeline()
{
    TBCI_Status status = tbci_context_stop(&tbciContext);
    return reportStatus(status, "stop");
}

// ---

bool tryGetTinyBCIInference(TinyBCIInference *out)
{
    if (eq_is_empty(&outputQueue))
        return false;

    TBCI_Epoch epoch;
    eq_pop(&outputQueue, &epoch);

    *out = (TinyBCIInference)
    {
        .predictedLabel = epoch.predicted_label,
        .targetLabel = epoch.label,
        .confidence = epoch.confidence
    };
    for (int i = 0; i < N_FREQS; i++)
    {
        out->confidences[i] = epoch.samples[i];
    }

    return true;
}