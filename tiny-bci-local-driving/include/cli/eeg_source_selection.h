# pragma once
# ifndef EEG_SOURCES
# define EEG_SOURCES

void runEEGSourceSelection();

void initializeSelectedEEGSource();
void updateSelectedEEGSource();
void cleanUpSelectedEEGSource();

typedef enum {
    LSLSource,
    NeuropawnSource,
    UnicornSource,
    DSI7Source,
    SyntheticSource
} EEGSourceType;

EEGSourceType promptEEGSourceSelection();
const char* promptSerialPortSelection();

bool isSelectedEEGSourceConnected();
uint8_t getChannelCountOfSelectedEEGSource();
uint32_t getSampleRateOfSelectedEEGSource();

# endif