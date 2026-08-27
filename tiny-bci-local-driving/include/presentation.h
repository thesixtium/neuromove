# pragma once
# include "raylib.h"
# include <stdbool.h>

# define RENDER_WIDTH 1024
# define RENDER_HEIGHT 600
# define MINIMUM_WINDOW_WIDTH 800
# define MINIMUM_WINDOW_HEIGHT 480

# define MESSAGE_SCREEN_FONT_SIZE 48

# define ROW_COUNT 2
# define MARGIN_TOP 25
# define MARGIN_BOTTOM 25
# define MARGIN_SIDE 25
# define GRID_GAP 200

# define SAFE_AREA_X (RENDER_WIDTH - 2 * MARGIN_SIDE)
# define SAFE_AREA_Y (RENDER_HEIGHT - (MARGIN_TOP + MARGIN_BOTTOM))

# define BACKGROUND_COLOUR (Color){32, 32, 32, 255}
# define LETTERBOX_COLOUR (Color){0, 0, 0, 255}
# define STIMULUS_BACKGROUND_COLOUR (Color){255, 255, 255, 255}
# define STIMULUS_ON_COLOUR (Color){255, 255, 255, 255}
# define STIMULUS_OFF_COLOUR (Color){0, 0, 0, 255}
# define STIMULUS_BREAK_PADDING 50

# define TARGET_INDICATION_OFFSET 20
# define TARGET_INDICATION_SIZE (Vector2){80, 50}
# define TARGET_INDICATION_COLOUR (Color){255, 80, 80, 255}

# define SELECTION_DISPLAY_WIDTH 20
# define SELECTION_DISPLAY_COLOUR (Color){120, 200, 120, 255}
# define SELECTION_DISPLAY_TIME 0.5f

# define TARGET_WIDTH 300
# define TARGET_HEIGHT 140

# define TAU 6.28318530717958647692528676655900576839433879875021

void initializePresentation(const float *, uint16_t);
void drawMessageScreen(const char*);
void drawStoppedScreen(void);
void drawStimulusScreen();
void stopPresentation();

void pauseStimulus();
void resumeStimulus();

void disableTextureStimulus();
void enableTextureStimulus();

void setPresentationTarget(uint16_t);
void clearPresentationTarget();
void displaySelection(uint16_t);

bool emergencyStopPressed(void);
