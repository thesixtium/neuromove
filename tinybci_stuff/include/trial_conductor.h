# pragma once

void initializeTrialConductor(uint16_t, uint16_t, float, float);

void setTrialStartCallback(void (*onTrialStart));
void setTrialEndCallback(void (*onTrialEnd));
void setStimulusRoundCompleteCallback(void (*onStimulusRoundCompleted));
void setAllTrialsCompletedCallback(void (*onAllTrialsCompleted));

void updateTrialConductor();
void resetTrialConductor();