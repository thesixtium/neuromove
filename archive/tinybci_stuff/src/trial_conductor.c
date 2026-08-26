# include "trial_conductor.h"
# include "microsecond_timer.h"

static void (*trialStartCallback)(uint16_t) = NULL;
static void (*trialEndCallback)(uint16_t) = NULL;
static void (*stimulusRoundCompletedCallback)(void) = NULL;
static void (*allTrialsCompletedCallback)(void) = NULL;

static MicrosecondTimer stimulusTimer;
static MicrosecondTimer breakTimer;

static uint16_t targetCount = 0;
static uint16_t target = 0;

static uint16_t stimulusRoundsToBeCompleted = 0;

enum State { BREAK, STIMULUS };
static uint16_t state = BREAK;


void initializeTrialConductor(
    uint16_t pTargetCount, uint16_t stimulusRoundsToComplete,
    float stimulusDuration, float breakDuration)
{
    targetCount = pTargetCount;
    stimulusRoundsToBeCompleted = stimulusRoundsToComplete;
    stimulusTimer = createMicrosecondTimer(stimulusDuration);
    breakTimer = createMicrosecondTimer(breakDuration);
}

void setTrialStartCallback(void (*onTrialStart)) { trialStartCallback = onTrialStart; }
void setTrialEndCallback(void (*onTrialEnd)) { trialEndCallback = onTrialEnd; }

void setStimulusRoundCompleteCallback(void (*onStimulusRoundCompleted))
{
    stimulusRoundCompletedCallback = onStimulusRoundCompleted;
}
void setAllTrialsCompletedCallback(void (*onTrialsCompleted))
{
    allTrialsCompletedCallback = onTrialsCompleted;
}

// ---

void startTrial()
{
    state = STIMULUS;
    if (trialStartCallback != NULL) trialStartCallback(target);
    resetMicrosecondTimer(&stimulusTimer);
}

void endTrial()
{
    state = BREAK;
    target = (target + 1) % targetCount;
    resetMicrosecondTimer(&breakTimer);

    if (trialEndCallback != NULL) trialEndCallback(target);
    if (target == 0) 
    {
        if (stimulusRoundCompletedCallback != NULL) stimulusRoundCompletedCallback();
        if (--stimulusRoundsToBeCompleted <= 0)
        {
            if (allTrialsCompletedCallback != NULL) allTrialsCompletedCallback();
        }
    }
}

void updateTrialConductor()
{
    switch (state)
    {
        case BREAK:
            if (checkMicrosecondTimer(&breakTimer)) startTrial();
            break;
        case STIMULUS:
            if (checkMicrosecondTimer(&stimulusTimer)) endTrial();
            break;
    }
}

// ---

void resetTrialConductor()
{
    resetMicrosecondTimer(&breakTimer);
    resetMicrosecondTimer(&stimulusTimer);
}