# pragma once
# include "storage.h"

# define SIGNAL_FREQUENCY 10.0f
# define SIGNAL_AMPLITUDE 1.0f
# define NOISE_AMPLITUDE 0.3f
# define NOISE_60HZ_AMPLITUDE 0.5f

void initializeSyntheticEEGSource(uint8_t, uint32_t);
void updateSyntheticEEGSource();
void cleanUpSyntheticEEGSource();
void resetSyntheticEEGSource();