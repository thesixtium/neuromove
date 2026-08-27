# include "program/helpers.h"
# include "program/constants.h"

# include "pipeline.h"
# include "trial_conductor.h"
# include "presentation.h"

# define MIN_VOTES_FOR_SELECTION 3

static int inferenceVotes[N_FREQS] = {0};

void initializeTrialPresentation(
    void (*trialStartCallback)(uint16_t),
    void (*trialEndCallback)(uint16_t),
    void (*allTrialsCompletedCallback)()
)
{
    initializeTrialConductor(
        N_FREQS, TRIAL_COUNT,
        STIMULUS_DURATION, BREAK_DURATION
    );
    setTrialStartCallback(trialStartCallback);
    setTrialEndCallback(trialEndCallback);
    setAllTrialsCompletedCallback(allTrialsCompletedCallback);

    initializePresentation(FREQUENCIES, N_FREQS);
    disableTextureStimulus();
}

void displayInference(TinyBCIInference inference, uint64_t timestamp)
{
    printInference(inference, timestamp);

    if (inference.confidence > SELECTION_DISPLAY_THRESHOLD && inference.predictedLabel < N_FREQS)
    {
        inferenceVotes[inference.predictedLabel]++;
    }
}

int finalizeTrialSelection(void)
{
    int winningSelection = -1;
    int mostVotes = 0;

    for (int i = 0; i < N_FREQS; i++)
    {
        if (inferenceVotes[i] > mostVotes)
        {
            mostVotes = inferenceVotes[i];
            winningSelection = i;
        }
    }

    if (winningSelection >= 0 && mostVotes >= MIN_VOTES_FOR_SELECTION)
    {
        printf(
            "Final trial selection: %d (%d votes)\n",
            winningSelection,
            mostVotes
        );

        displaySelection(winningSelection);
    }
    else
    {
        printf("No selection: only %d votes\n", mostVotes);
        winningSelection = -1;
        }

    /*
     * Reset vote counts for the next trial.
     */
    for (int i = 0; i < N_FREQS; i++)
    {
        inferenceVotes[i] = 0;
    }
    
    return winningSelection;
}



void displayMessageOrExit(const char *message, void (*cleanUpMethod)())
{
    drawMessageScreen(message);
    closeIfPromptedTo(cleanUpMethod);
}

void closeIfPromptedTo(void (*cleanUpMethod)())
{
    if (WindowShouldClose())
    {
        if (cleanUpMethod != NULL) cleanUpMethod();
        exit(EXIT_SUCCESS);
    }
}
