# include "cli/program_mode_selection.h"
# include "cli/eeg_source_selection.h"

# include "raylib.h"

#include "data/dsi_eeg_source.h"
#include "presentation.h"

# include <stdbool.h>

static bool stopped = false;

int main(int argc, char *argv[])
{
    ProgramMode mode = promptProgramModeSelection();
    if (mode == Standalone) runEEGSourceSelection();

    initializeProgram(mode);
    awaitPromptedProgramStart();

    while (!WindowShouldClose())
    {
        if (!stopped)
        {
            updateProgram();

            if (emergencyStopPressed())
            {
                /*
                 * IMPORTANT:
                 * Write STOP to pookinator here

                /*
                 * Stop the SSVEP flicker.
                 */
                disableTextureStimulus();

                /*
                 * Stop further BCI processing.
                 */
                stopped = true;
            }
        }
        else
        {
            drawStoppedScreen();
        }
    }

    cleanUpProgram();
    return EXIT_SUCCESS;
}
