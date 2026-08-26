# pragma once
# ifndef PROGRAM_MODES
# define PROGRAM_MODES

typedef enum {
    Standalone,
    PresentationOnly
} ProgramMode;

ProgramMode promptProgramModeSelection();

void initializeProgram(ProgramMode mode);
void awaitPromptedProgramStart();
void updateProgram();
void cleanUpProgram();

# endif