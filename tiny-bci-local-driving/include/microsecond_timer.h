# pragma once
uint64_t getCurrentMicrosecondTimestamp();

typedef struct {
    uint64_t interval;
    uint64_t nextTimeout;
} MicrosecondTimer;

bool checkMicrosecondTimer(MicrosecondTimer*);
void resetMicrosecondTimer(MicrosecondTimer*);

MicrosecondTimer createMicrosecondTimer(float);