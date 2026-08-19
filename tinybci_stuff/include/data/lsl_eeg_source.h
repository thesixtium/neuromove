# pragma once
# ifndef LSL_EEG_SOURCE
# define LSL_EEG_SOURCE

# define LSL_SCAN_TIMEOUT 1.0
# define LSL_CONNECT_TIMEOUT 2.0
# define LSL_EEG_PREDICATE "type='EEG' or type='eeg'"

void connectLslEEGSource();
void updateLslEEGSource();
void disconnectLslEEGSource();

uint8_t getLslEEGSourceChannelCount();
uint32_t getLslEEGSourceSampleRate();

# endif