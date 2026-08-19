# include "microsecond_timer.h"

# if defined(_WIN32) || defined(_WIN64)

# define WIN32_LEAN_AND_MEAN
# include <windows.h>
uint64_t getCurrentMicrosecondTimestamp()
{
    FILETIME fileTime;
    uint64_t time;

    GetSystemTimePreciseAsFileTime(&fileTime);
    time = fileTime.dwLowDateTime;
    time += ((uint64_t)fileTime.dwHighDateTime) << 32;
    
    return time / 10;
}

# else

# include <time.h>
uint64_t getCurrentMicrosecondTimestamp()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

# endif

bool checkMicrosecondTimer(MicrosecondTimer *timer)
{
    uint64_t now = getCurrentMicrosecondTimestamp();
    if (timer->nextTimeout == 0) timer->nextTimeout = now;

    if (now >= timer->nextTimeout)
    {
        timer->nextTimeout += timer->interval;
        return true;
    }
    return false;
}

void resetMicrosecondTimer(MicrosecondTimer *timer)
{
    uint64_t now = getCurrentMicrosecondTimestamp();
    timer->nextTimeout = now + timer->interval;
}

// ---

MicrosecondTimer createMicrosecondTimer(float intervalSeconds)
{
    return (MicrosecondTimer){
        .interval = (uint64_t)(intervalSeconds * 1000000),
        .nextTimeout = getCurrentMicrosecondTimestamp()
    };
}