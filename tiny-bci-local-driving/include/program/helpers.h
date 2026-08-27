# pragma once
# ifndef PROGRAM_FRAGMENTS
# define PROGRAM_FRAGMENTS

# include "pipeline.h"

void initializeTrialPresentation(
    void (*trialStartCallback)(uint16_t),
    void (*trialEndCallback)(uint16_t),
    void (*allTrialsCompletedCallback)()
);

void initializePipelineWithEEGSourceParameters();
void updateEEGSourceAndPipeline(void (*cleanUpMethod)());
void cleanUpEEGSourceAndPipeline();

void awaitFilterStabilization(void (*cleanUpMethod)());
void awaitConnection(
    bool (*predicate)(),
    void (*updateMethod)(),
    bool (*attemptMethod)()
);

void displayInference(TinyBCIInference inference, uint64_t timestamp);
void finalizeTrialSelection(void);
void printInference(TinyBCIInference inference, uint64_t timestamp);

void displayMessageOrExit(const char *message, void (*cleanUpMethod)());
void closeIfPromptedTo(void (*cleanUpMethod)());

# endif
