# pragma once

void initializeTrialConductor(uint16_t, uint16_t, float, float);

void setTrialStartCallback(void (*onTrialStart)(uint16_t));
void setTrialEndCallback(void (*onTrialEnd)(uint16_t));
void setStimulusRoundCompleteCallback(void (*onStimulusRoundCompleted));
void setAllTrialsCompletedCallback(void (*onAllTrialsCompleted));

void updateTrialConductor();
void resetTrialConductor();