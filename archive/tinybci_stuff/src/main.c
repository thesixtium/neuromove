#include "pipeline.h"
#include "presentation.h"
#include "trial_conductor.h"
#include "microsecond_timer.h"
#include <string.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include "inference_logger.h"

#include "data/trigger_source.h"
#include "data/lsl_trigger_outlet.h"

#include "data/dsi_eeg_source.h"

/* Set to your headset's serial port (e.g. "/dev/ttyACM0", "COM4"), or
 * NULL to fall back to the DSISerialPort environment variable. */
#define DSI_SERIAL_PORT "/dev/ttyUSB0"
#define DSI_MONTAGE "F4,C4,S3,S1,C3,F3"

void initializeEEGSource(void) { connectDsiEEGSource(DSI_SERIAL_PORT, DSI_MONTAGE); }
void updateEEGSource(void) { updateDsiEEGSource(); }
void cleanUpEEGSource(void) { disconnectDsiEEGSource(); }

/* Reported by the headset itself once connected -- reflects the
 * montage above, not a fixed constant. */
uint8_t getChannelCount(void) { return getDsiEEGSourceChannelCount(); }
uint32_t getSampleRate(void) { return getDsiEEGSourceSampleRate(); }


static uint16_t currentTargetLabel = 0;
void onTrialStart(uint16_t target)
{
    currentTargetLabel = target;
    pushTrigger(target + 1);
    pushLslTrigger(target + 1);
    setPresentationTarget(target);
    resumeStimulus();
}

void onTrialEnd(uint16_t nextTarget)
{
    pushTrigger(TRIAL_END_CODE);
    pushLslTrigger(TRIAL_END_CODE);
    setPresentationTarget(nextTarget);
    pauseStimulus();
}

static bool allTrialsCompleted = false;
void onAllTrialsCompleted(void)
{
    allTrialsCompleted = true;
    clearPresentationTarget();
}


int main(int argc, char *argv[])
{
    const float frequencies[N_FREQS] = {7.5f, 8.57f, 10.0f, 12.0f};

    const uint16_t stimulusRounds = 1000;

    const float filterStabilizationDelay = 5.0f;
    const float stimulusDuration = 4.0f;
    const float breakDuration = 4.0f;

    const float selectionDisplayConfidenceThreshold = 0.9f;

    initializeTrialConductor(N_FREQS, stimulusRounds, stimulusDuration, breakDuration);
    setTrialStartCallback(onTrialStart);
    setTrialEndCallback(onTrialEnd);
    setAllTrialsCompletedCallback(onAllTrialsCompleted);

    initializePresentation(frequencies, N_FREQS);
    setPresentationTarget(0);
    disableTextureStimulus();

    initializeEEGSource();
    openLslTriggerOutlet("tBCI_Experiment_Triggers");

    uint8_t channelCount = getChannelCount();
    uint32_t sampleRate = getSampleRate();
    if (initializeTinyBCIPipeline(frequencies, channelCount, sampleRate)) return EXIT_FAILURE;

    if (startTinyBCIPipeline()) return EXIT_FAILURE;
    printf("---\nTiny BCI Pipeline Running.\n\n");

    MicrosecondTimer stabilizationTimer = createMicrosecondTimer(filterStabilizationDelay);
    resetMicrosecondTimer(&stabilizationTimer);
    while (!checkMicrosecondTimer(&stabilizationTimer))
    {
        drawMessageScreen("Awaiting Filter Stabilization...");
        updateEEGSource();
        updateTinyBCIPipeline();

        if (WindowShouldClose())
        {
            cleanUpEEGSource();
            closeLslTriggerOutlet();
            stopPresentation();
            return EXIT_SUCCESS;
        }
    }
    printf("Filter settled.\n");

    while (!IsKeyPressed(KEY_SPACE))
    {
        drawMessageScreen("Press Spacebar to Start");
        updateEEGSource();

        if (WindowShouldClose())
        {
            cleanUpEEGSource();
            closeLslTriggerOutlet();
            stopPresentation();
            return EXIT_SUCCESS;
        }
    }
    initializeInferenceLogger();

    /* Shared Memory Setup */
    size_t shm_size = 284622;
    const char *shmem_name = "pookinator";
    int shmem_fd = shm_open(shmem_name, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (shmem_fd == -1) { perror("shm_open"); return 1; }
    printf("Shared Memory segment opened with fd %d\n", shmem_fd);
    if (ftruncate(shmem_fd, shm_size) == -1) { perror("ftruncate"); return 1; }
    printf("Shared Memory segment resized to %zu\n", shm_size);
    void *addr = mmap(0, shm_size, PROT_WRITE, MAP_SHARED, shmem_fd, 0);
    if (addr == MAP_FAILED) { perror("mmap"); return 1; }

    while (!WindowShouldClose())
    {
        if (allTrialsCompleted)
        {
            drawMessageScreen("Experiment Complete");
            continue;
        }

        updateEEGSource();
        updateTrialConductor();

        if (updateTinyBCIPipeline()) break;

        TinyBCIInference inference;
        if (tryGetTinyBCIInference(&inference))
        {
            uint64_t timestamp = getCurrentMicrosecondTimestamp();
            printf("%" PRIu64 " | Output received: %d (%.0f%% confidence) [", timestamp,
                inference.predictedLabel, inference.confidence * 100
            );
            for (int i = 0; i < N_FREQS; i++) printf(" %.2f", inference.confidences[i]);
            printf(" ]\n");

            logInference(inference, timestamp, currentTargetLabel);

            if (inference.confidence > selectionDisplayConfidenceThreshold)
            {
                displaySelection(inference.predictedLabel);
            }

            /* Shared memory
             * 0: Stop
             * 1: Forward
             * 2: Right
             * 3: Left
             */
            printf("SMS\n");
            {
                /* read_local_driving() on the Python side calls read_string(),
                 * which does bytes(buf).strip(b'\x00').decode() over the WHOLE
                 * buffer -- so we need an actual ASCII string in shared memory,
                 * not a raw numeric byte. It also expects the labels as plain
                 * digit strings ("1", "2", "3"; anything else -> STOP).
                 *
                 * Clear the buffer first so a shorter new label fully
                 * overwrites a longer previous one (otherwise leftover
                 * trailing digits/nulls from the last write could remain in
                 * the middle of the buffer and corrupt the strip/decode). */
                char labelStr[16];
                int labelLen = snprintf(labelStr, sizeof(labelStr), "%d", inference.predictedLabel);
                memset(addr, 0, shm_size);
                memcpy(addr, labelStr, (size_t)labelLen);
            }
            printf("SME\n");
        }

        drawStimulusScreen();
    }

    cleanUpEEGSource();
    cleanUpTinyBCIPipeline();
    closeLslTriggerOutlet();
    closeInferenceLogger();
    stopPresentation();

    return EXIT_SUCCESS;
}
