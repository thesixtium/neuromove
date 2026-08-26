# include "cli/program_mode_selection.h"
# include "cli/helpers.h"

# include "program/constants.h"
# include "program/helpers.h"
# include "presentation.h"
# include "trial_conductor.h"
# include "microsecond_timer.h"

# include "pipeline.h"
# include "triggers.h"
# include "lsl/trigger_stream.h"
# include "lsl/inference_stream.h"
# include "inference_logger.h"

# include "cli/eeg_source_selection.h"

static ProgramMode programMode;
# define STANDALONE(code) if(programMode == Standalone) { code }
# define PRESENTATION_ONLY(code) if(programMode == PresentationOnly) { code }


// ---
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
// ---

static const size_t shm_size = 284622;
static const char *shmem_name = "pookinator";
static void *addr;

ProgramMode promptProgramModeSelection()
{
    return Standalone;
    printf("Select program mode\n");
    printf("\t%u - Standalone\n", Standalone);
    printf(
        "\t%u - Presentation Only "
        "(connected to headless engine over LSL)\n"
        , PresentationOnly
    );

    uint32_t selection = getIntegerSelection(PresentationOnly);

    printf(
        "\nProceeding in %s mode...\n",
        selection == Standalone ? "Standalone" : "Presentation Only"
    );
    return (ProgramMode)selection;
}

// ---

static uint16_t currentTargetLabel = 0;
void onTrialStart(uint16_t target)
{
    currentTargetLabel = target;
    STANDALONE(pushTrigger(target + 1);)
    pushLslTrigger(target + 1);
    setPresentationTarget(target);
    resumeStimulus();
}

void onTrialEnd(uint16_t nextTarget)
{
    STANDALONE(pushTrigger(TRIAL_END_CODE);)
    pushLslTrigger(TRIAL_END_CODE);
    setPresentationTarget(nextTarget);
    pauseStimulus();
}

static bool allTrialsCompleted = false;
void onAllTrialsCompleted()
{
    allTrialsCompleted = true;
    clearPresentationTarget();
}

bool tryGetInference(TinyBCIInference *inference, uint64_t *timestamp)
{
    STANDALONE(
        *timestamp = getCurrentMicrosecondTimestamp();
        return tryGetTinyBCIInference(inference);
    )
    PRESENTATION_ONLY(
        return pollLslInferenceSource(inference, timestamp);
    )
    return false;
}

// ---

void displayHeadlessRuntimeConnectionWaitMessage()
{
    displayMessageOrExit("Waiting for BCI Engine...", &cleanUpProgram);
}

void connectToHeadlessRuntime()
{
    while (!doesLslTriggerOutletHaveConsumers())
    {
        displayMessageOrExit("Searching for BCI Engine...", &cleanUpProgram);
    }

    initializeLslInferenceSource();

    awaitConnection(
        &isLslInferenceSourceConnected,
        &displayHeadlessRuntimeConnectionWaitMessage,
        &tryConnectLslInferenceSource
    );
}

// ---

void initializeProgram(ProgramMode mode)
{
    programMode = mode;
    initializeTrialPresentation(
        &onTrialStart, &onTrialEnd,
        &onAllTrialsCompleted
    );
    STANDALONE(
        drawMessageScreen("Initializing EEG Source...");
        initializeSelectedEEGSource();
    )
    openLslTriggerOutlet();

    STANDALONE(
        initializePipelineWithEEGSourceParameters();
        drawMessageScreen("Awaiting Filter Stabilization...");
        awaitFilterStabilization(&cleanUpProgram);
    )

    PRESENTATION_ONLY(connectToHeadlessRuntime();)
}

void awaitPromptedProgramStart()
{
    while (!IsKeyPressed(KEY_SPACE))
    {
        drawMessageScreen("Press Spacebar to Start");
        STANDALONE(
            updateEEGSourceAndPipeline(&cleanUpProgram);
            if (!isSelectedEEGSourceConnected()) return;
        )
        closeIfPromptedTo(&cleanUpProgram);
    }
    
    
    /* Shared Memory Setup */
    int shmem_fd = shm_open(shmem_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (shmem_fd == -1) { perror("shm_open"); exit(1); }
    printf("Shared Memory segment opened with fd %d\n", shmem_fd);
    if (ftruncate(shmem_fd, shm_size) == -1) { perror("ftruncate"); exit(1); }
    printf("Shared Memory segment resized to %zu\n", shm_size);
    addr = mmap(0, shm_size, PROT_WRITE, MAP_SHARED, shmem_fd, 0);
    if (addr == MAP_FAILED) { perror("mmap"); exit(1); }
}

void updateProgram()
{
    if (allTrialsCompleted)
    {
        drawMessageScreen("Experiment Complete");
        return;
    }

    PRESENTATION_ONLY(
        if (!isLslInferenceSourceConnected())
        {
            drawMessageScreen("Connection Lost");
            return;
        }
    )

    STANDALONE(
        if (!isSelectedEEGSourceConnected())
        {
            drawMessageScreen("EEG Source Disconnected");
            return;
        }
    )

    updateTrialConductor();
    STANDALONE(updateEEGSourceAndPipeline(&cleanUpProgram);)

    TinyBCIInference inference;
    uint64_t timestamp;
    if (tryGetInference(&inference, &timestamp))
    {
        displayInference(inference, timestamp);
        STANDALONE(logInference(inference, timestamp, currentTargetLabel);)
        
        /* Shared memory
         * 0: Stop
         * 1: Forward
         * 2: Right
         * 3: Left
         */
        
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
    }

    drawStimulusScreen();
}

void cleanUpProgram()
{
    STANDALONE(cleanUpEEGSourceAndPipeline();)
    PRESENTATION_ONLY(closeLslInferenceSource();)
    closeLslTriggerOutlet();
    stopPresentation();
}
