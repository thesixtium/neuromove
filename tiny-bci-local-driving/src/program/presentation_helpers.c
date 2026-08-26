# include "program/helpers.h"
# include "program/constants.h"

# include "pipeline.h"
# include "trial_conductor.h"
# include "presentation.h"

void initializeTrialPresentation(
    void (*trialStartCallback)(uint16_t),
    void (*trialEndCallback)(uint16_t),
    void (*allTrialsCompletedCallback)()
)
{
    initializeTrialConductor(
        N_FREQS, TRIAL_COUNT,
        STIMULUS_DURATION, BREAK_DURATION
    );
    setTrialStartCallback(trialStartCallback);
    setTrialEndCallback(trialEndCallback);
    setAllTrialsCompletedCallback(allTrialsCompletedCallback);

    initializePresentation(FREQUENCIES, N_FREQS);
    setPresentationTarget(0);
    disableTextureStimulus();
}

void displayInference(TinyBCIInference inference, uint64_t timestamp)
{
    printInference(inference, timestamp);

    if (inference.confidence > SELECTION_DISPLAY_THRESHOLD)
    {
        displaySelection(inference.predictedLabel);
    }
}

void displayMessageOrExit(const char *message, void (*cleanUpMethod)())
{
    drawMessageScreen(message);
    closeIfPromptedTo(cleanUpMethod);
}

void closeIfPromptedTo(void (*cleanUpMethod)())
{
    if (WindowShouldClose())
    {
        if (cleanUpMethod != NULL) cleanUpMethod();
        exit(EXIT_SUCCESS);
    }
}