/**
* @file tbci_platform.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Platform abstraction utilities for TinyBCI.
 *
 * Provides portable implementations of common system calls
 * that differ across Windows, macOS, Linux, and embedded targets.
 */

#ifndef TBCI_PLATFORM_H
#define TBCI_PLATFORM_H

#if defined(_WIN32) || defined(_WIN64)
#   include <windows.h>
#elif defined(__APPLE__) || defined(__linux__) || defined(__unix__)
#   include <time.h>
#elif defined(__arm__) || defined(__ARM_ARCH)
    /* bare-metal ARM — implement tbci_sleep_us via hardware timer */
#endif

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * @brief Sleep for approximately the given number of microseconds.
     *
     * Resolution varies by platform:
     * - Windows: ~1ms (Sleep granularity)
     * - macOS/Linux: ~100ns (nanosleep)
     * - Bare-metal: hardware timer dependent
     *
     * @param[in] us  Microseconds to sleep.
     */
    static inline void tbci_sleep_us(unsigned int us)
    {
#if defined(_WIN32) || defined(_WIN64)
        Sleep(us / 1000);
#elif defined(__APPLE__) || defined(__linux__) || defined(__unix__)
        struct timespec ts;
        ts.tv_sec  = us / 1000000;
        ts.tv_nsec = (long)((us % 1000000) * 1000);
        nanosleep(&ts, NULL);
#else
        /* bare-metal — no-op, caller uses hardware timer */
        (void)us;
#endif
    }

#ifdef __cplusplus
}
#endif

#endif /* TBCI_PLATFORM_H */