#include "presentation.h"

static RenderTexture2D renderTarget;
static Texture2D renderTexture;
static Rectangle renderTextureRect;

static float *frequencies;
static uint16_t frequencyCount;

static uint16_t targetIndex;
static bool hasTarget = false;

static uint16_t selectionIndex;
static double selectionTime = -SELECTION_DISPLAY_TIME;

static bool stimulusEnabled = true;
static bool textureEnabled = true;

#define MIN(a, b) ((a) < (b) ? (a) : (b))


/* ---------------------------------------------------------
 * NeuroMove command mapping
 *
 * This MUST remain consistent with the TinyBCI → NeuroMove
 * shared-memory mapping:
 *
 * 0 = STOP
 * 1 = FORWARD
 * 2 = RIGHT
 * 3 = LEFT
 * ---------------------------------------------------------
 */

typedef enum
{
    COMMAND_STOP    = 0,
    COMMAND_FORWARD = 1,
    COMMAND_RIGHT   = 2,
    COMMAND_LEFT    = 3
} WheelchairCommand;


/*
 * Fixed rectangles for the four SSVEP targets.
 *
 * Instead of using the old generic grid layout, each target
 * now has an explicitly defined screen location.
 */
static Rectangle presenterRects[4];


/* =========================================================
 * Window setup
 * =========================================================
 */

void initializeWindow()
{
    SetTraceLogLevel(LOG_WARNING);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    InitWindow(
        RENDER_WIDTH,
        RENDER_HEIGHT,
        "Tiny BCI SSVEP Experiment"
    );

    SetWindowMinSize(
        MINIMUM_WINDOW_WIDTH,
        MINIMUM_WINDOW_HEIGHT
    );

    renderTarget =
        LoadRenderTexture(
            RENDER_WIDTH,
            RENDER_HEIGHT
        );

    renderTexture =
        renderTarget.texture;

    SetTextureFilter(
        renderTexture,
        TEXTURE_FILTER_POINT
    );

    renderTextureRect =
        (Rectangle)
        {
            0,
            0,
            RENDER_WIDTH,
            -RENDER_HEIGHT
        };
}


/* =========================================================
 * Presenter layout
 * =========================================================
 */

void initializePresenters(
    const float *pFrequencies,
    uint16_t pFrequencyCount
)
{
    frequencyCount =
        pFrequencyCount;

    size_t memorySize =
        frequencyCount * sizeof(float);

    frequencies =
        malloc(memorySize);

    memcpy(
        frequencies,
        pFrequencies,
        memorySize
    );


    /*
     * Fixed NeuroMove cross layout:
     *
     *
     *               FORWARD
     *
     *
     *        LEFT             RIGHT
     *
     *
     *                STOP
     *
     *
     * All four targets are the SAME size.
     */

    float centreX =
        RENDER_WIDTH / 2.0f;

    float centreY =
        RENDER_HEIGHT / 2.0f;


    /*
     * STOP
     * Bottom middle
     */
    presenterRects[COMMAND_STOP] =
        (Rectangle)
        {
            centreX -
                STANDARD_TARGET_SIZE / 2.0f,

            RENDER_HEIGHT -
                MARGIN_BOTTOM -
                STANDARD_TARGET_SIZE,

            STANDARD_TARGET_SIZE,
            STANDARD_TARGET_SIZE
        };


    /*
     * FORWARD
     * Top middle
     */
    presenterRects[COMMAND_FORWARD] =
        (Rectangle)
        {
            centreX -
                STANDARD_TARGET_SIZE / 2.0f,

            MARGIN_TOP,

            STANDARD_TARGET_SIZE,
            STANDARD_TARGET_SIZE
        };


    /*
     * RIGHT
     * Right side, vertically centred
     */
    presenterRects[COMMAND_RIGHT] =
        (Rectangle)
        {
            RENDER_WIDTH -
                MARGIN_SIDE -
                STANDARD_TARGET_SIZE,

            centreY -
                STANDARD_TARGET_SIZE / 2.0f,

            STANDARD_TARGET_SIZE,
            STANDARD_TARGET_SIZE
        };


    /*
     * LEFT
     * Left side, vertically centred
     */
    presenterRects[COMMAND_LEFT] =
        (Rectangle)
        {
            MARGIN_SIDE,

            centreY -
                STANDARD_TARGET_SIZE / 2.0f,

            STANDARD_TARGET_SIZE,
            STANDARD_TARGET_SIZE
        };
}


void initializePresentation(
    const float *pFrequencies,
    uint16_t pFrequencyCount
)
{
    initializeWindow();

    initializePresenters(
        pFrequencies,
        pFrequencyCount
    );
}


/* =========================================================
 * Target geometry helpers
 * =========================================================
 */

Vector2 getGridOrigin(
    uint16_t index
)
{
    return (Vector2)
    {
        presenterRects[index].x,
        presenterRects[index].y
    };
}


Vector2 getGridCentre(
    uint16_t index
)
{
    Rectangle r =
        presenterRects[index];

    return (Vector2)
    {
        r.x + r.width / 2.0f,
        r.y + r.height / 2.0f
    };
}


Rectangle getGridRect(
    uint16_t index,
    int16_t padding
)
{
    Rectangle r =
        presenterRects[index];

    return (Rectangle)
    {
        r.x - padding,
        r.y - padding,

        r.width +
            2 * padding,

        r.height +
            2 * padding
    };
}


/* =========================================================
 * Target indicator
 * =========================================================
 */

void setPresentationTarget(
    uint16_t index
)
{
    targetIndex =
        index;

    hasTarget =
        true;
}


void clearPresentationTarget()
{
    hasTarget =
        false;
}


void drawTargetIndicator()
{
    if (!hasTarget)
    {
        return;
    }

    Vector2 gridCentre =
        getGridCentre(
            targetIndex
        );

    Vector2 arrowTip =
        gridCentre;

    arrowTip.y +=
        presenterRects[targetIndex].height
        / 2.0f;

    arrowTip.y +=
        TARGET_INDICATION_OFFSET;


    Vector2 arrowBottomLeft =
        (Vector2)
        {
            arrowTip.x -
                TARGET_INDICATION_SIZE.x
                / 2.0f,

            arrowTip.y +
                TARGET_INDICATION_SIZE.y
        };


    Vector2 arrowBottomRight =
        (Vector2)
        {
            arrowBottomLeft.x +
                TARGET_INDICATION_SIZE.x,

            arrowBottomLeft.y
        };


    DrawTriangle(
        arrowTip,
        arrowBottomLeft,
        arrowBottomRight,
        TARGET_INDICATION_COLOUR
    );
}


/* =========================================================
 * Selection feedback
 * =========================================================
 */

void displaySelection(
    uint16_t index
)
{
    selectionIndex =
        index;

    selectionTime =
        GetTime();
}


void drawSelectionIndicator()
{
    if (
        GetTime() >
        selectionTime +
        SELECTION_DISPLAY_TIME
    )
    {
        return;
    }

    Rectangle borderRect =
        getGridRect(
            selectionIndex,
            SELECTION_DISPLAY_WIDTH
        );

    DrawRectangleRec(
        borderRect,
        SELECTION_DISPLAY_COLOUR
    );
}


/* =========================================================
 * Letterboxing / window scaling
 * =========================================================
 */

void drawLetterboxedTarget()
{
    BeginDrawing();

        ClearBackground(
            LETTERBOX_COLOUR
        );


        float scaleX =
            (float)GetScreenWidth()
            / RENDER_WIDTH;

        float scaleY =
            (float)GetScreenHeight()
            / RENDER_HEIGHT;

        float scale =
            MIN(
                scaleX,
                scaleY
            );


        Rectangle letterboxRect =
        {
            (
                GetScreenWidth()
                -
                scale * RENDER_WIDTH
            ) / 2.0f,

            (
                GetScreenHeight()
                -
                scale * RENDER_HEIGHT
            ) / 2.0f,

            RENDER_WIDTH * scale,
            RENDER_HEIGHT * scale
        };


        DrawTexturePro(
            renderTexture,
            renderTextureRect,
            letterboxRect,
            (Vector2){ 0, 0 },
            0,
            WHITE
        );

    EndDrawing();
}


/* =========================================================
 * Message screen
 * =========================================================
 */

void drawMessageScreen(
    const char *message
)
{
    BeginTextureMode(
        renderTarget
    );

        ClearBackground(
            BACKGROUND_COLOUR
        );


        for (
            uint16_t i = 0;
            i < frequencyCount;
            i++
        )
        {
            DrawRectangleRec(
                getGridRect(
                    i,
                    -STIMULUS_BREAK_PADDING
                ),
                STIMULUS_BACKGROUND_COLOUR
            );
        }


        drawTargetIndicator();


        int textWidth =
            MeasureText(
                message,
                MESSAGE_SCREEN_FONT_SIZE
            );


        DrawText(
            message,

            (
                RENDER_WIDTH -
                textWidth
            ) / 2,

            RENDER_HEIGHT / 2,

            MESSAGE_SCREEN_FONT_SIZE,

            STIMULUS_BACKGROUND_COLOUR
        );

    EndTextureMode();


    drawLetterboxedTarget();
}


/* =========================================================
 * NeuroMove stimulus symbols
 * =========================================================
 */


/*
 * Rotate a Vector2 around the origin.
 *
 * Positive rotation appears clockwise in raylib's
 * screen coordinate system because +Y points downward.
 */
static Vector2 rotateVector2(
    Vector2 p,
    float rotationDeg
)
{
    double rad =
        rotationDeg * DEG2RAD;

    double cosA =
        cos(rad);

    double sinA =
        sin(rad);


    return (Vector2)
    {
        (float)(
            p.x * cosA -
            p.y * sinA
        ),

        (float)(
            p.x * sinA +
            p.y * cosA
        )
    };
}


/*
 * Draw STOP as an octagonal outline.
 */
static void drawStopOutline(
    Vector2 centre,
    float size,
    Color color,
    float lineThick
)
{
    DrawPolyLinesEx(
        centre,
        8,
        size / 2.0f,
        22.5f,
        lineThick,
        color
    );
}


/*
 * Draw an outlined arrow.
 *
 * Before rotation, the arrow points upward.
 *
 * rotationDeg:
 *
 *   0   = FORWARD
 *   90  = RIGHT
 *  -90  = LEFT
 */
static void drawArrowOutline(
    Vector2 centre,
    float size,
    float rotationDeg,
    Color color,
    float lineThick
)
{
    float headWidth =
        size * 0.65f;

    float headHeight =
        size * 0.45f;

    float shaftWidth =
        size * 0.28f;

    float shaftLength =
        size * 0.55f;


    Vector2 tip =
    {
        0,
        -size / 2.0f
    };


    Vector2 headLeft =
    {
        -headWidth / 2.0f,
        tip.y + headHeight
    };


    Vector2 headRight =
    {
        headWidth / 2.0f,
        tip.y + headHeight
    };


    Vector2 shaftTopLeft =
    {
        -shaftWidth / 2.0f,
        headLeft.y
    };


    Vector2 shaftTopRight =
    {
        shaftWidth / 2.0f,
        headLeft.y
    };


    Vector2 shaftBottomLeft =
    {
        -shaftWidth / 2.0f,
        headLeft.y + shaftLength
    };


    Vector2 shaftBottomRight =
    {
        shaftWidth / 2.0f,
        headLeft.y + shaftLength
    };


    Vector2 outline[7] =
    {
        tip,
        headLeft,
        shaftTopLeft,
        shaftBottomLeft,
        shaftBottomRight,
        shaftTopRight,
        headRight
    };


    /*
     * Rotate and translate each point.
     */
    for (
        int i = 0;
        i < 7;
        i++
    )
    {
        outline[i] =
            rotateVector2(
                outline[i],
                rotationDeg
            );

        outline[i].x +=
            centre.x;

        outline[i].y +=
            centre.y;
    }


    /*
     * Join the seven outline points.
     */
    for (
        int i = 0;
        i < 7;
        i++
    )
    {
        Vector2 next =
            outline[
                (i + 1) % 7
            ];

        DrawLineEx(
            outline[i],
            next,
            lineThick,
            color
        );
    }
}


/*
 * Draw the appropriate symbol for each class.
 */
static void drawStimulusSymbol(
    uint16_t index,
    Rectangle gridRect,
    Color color,
    float lineThick
)
{
    Vector2 centre =
    {
        gridRect.x +
            gridRect.width / 2.0f,

        gridRect.y +
            gridRect.height / 2.0f
    };


    float symbolSize =
        MIN(
            gridRect.width,
            gridRect.height
        ) * 0.6f;


    switch (index)
    {
        case COMMAND_STOP:

            drawStopOutline(
                centre,
                symbolSize,
                color,
                lineThick
            );

            break;


        case COMMAND_FORWARD:

            drawArrowOutline(
                centre,
                symbolSize,
                0.0f,
                color,
                lineThick
            );

            break;


        case COMMAND_RIGHT:

            drawArrowOutline(
                centre,
                symbolSize,
                90.0f,
                color,
                lineThick
            );

            break;


        case COMMAND_LEFT:

            drawArrowOutline(
                centre,
                symbolSize,
                -90.0f,
                color,
                lineThick
            );

            break;


        default:

            DrawRectangleLinesEx(
                (Rectangle)
                {
                    centre.x -
                        symbolSize / 2.0f,

                    centre.y -
                        symbolSize / 2.0f,

                    symbolSize,
                    symbolSize
                },
                lineThick,
                color
            );

            break;
    }
}


/* =========================================================
 * Individual SSVEP stimulus
 * =========================================================
 */

void drawStimulusPresenter(
    uint16_t index
)
{
    Rectangle gridRect =
        getGridRect(
            index,
            0
        );


    /*
     * IMPORTANT:
     *
     * This is the existing TinyBCI SSVEP flicker waveform.
     * Do not change this as part of the UI redesign.
     */
    double waveValue =
        sin(
            frequencies[index]
            *
            TAU
            *
            GetTime()
        );


    /*
     * Avoid division by zero at the exact zero crossing.
     */
    double weightedValue =
        0.0;

    if (waveValue != 0.0)
    {
        weightedValue =
            sqrt(
                fabs(waveValue)
            )
            *
            (
                waveValue
                /
                fabs(waveValue)
            );
    }


    float normalizedValue =
        (float)(
            weightedValue + 1.0
        )
        / 2.0f;


    /*
     * Full-field flicker.
     *
     * The ENTIRE square target changes luminance,
     * preserving the large SSVEP stimulation area.
     */
    Color stimulusColor =
        ColorLerp(
            STIMULUS_OFF_COLOUR,
            STIMULUS_ON_COLOUR,
            normalizedValue
        );


    DrawRectangleRec(
        gridRect,
        stimulusColor
    );


    /*
     * Draw the command symbol using the opposite colour
     * so it remains visible throughout both phases of
     * the flicker.
     */
    Color outlineColor =
        (
            normalizedValue > 0.5f
        )
        ?
        STIMULUS_OFF_COLOUR
        :
        STIMULUS_ON_COLOUR;


    float lineThick =
        textureEnabled
        ?
        8.0f
        :
        4.0f;


    drawStimulusSymbol(
        index,
        gridRect,
        outlineColor,
        lineThick
    );
}


/* =========================================================
 * Main stimulus screen
 * =========================================================
 */

void drawStimulusScreen()
{
    BeginTextureMode(
        renderTarget
    );

        ClearBackground(
            BACKGROUND_COLOUR
        );


        drawSelectionIndicator();


        for (
            uint16_t i = 0;
            i < frequencyCount;
            i++
        )
        {
            if (stimulusEnabled)
            {
                drawStimulusPresenter(
                    i
                );
            }
            else
            {
                DrawRectangleRec(
                    getGridRect(
                        i,
                        -STIMULUS_BREAK_PADDING
                    ),
                    STIMULUS_BACKGROUND_COLOUR
                );
            }
        }


        drawTargetIndicator();

    EndTextureMode();


    drawLetterboxedTarget();
}


/* =========================================================
 * Stimulus control
 * =========================================================
 */

void pauseStimulus()
{
    stimulusEnabled =
        false;
}


void resumeStimulus()
{
    stimulusEnabled =
        true;
}


void disableTextureStimulus()
{
    textureEnabled =
        false;
}


void enableTextureStimulus()
{
    textureEnabled =
        true;
}


/* =========================================================
 * Cleanup
 * =========================================================
 */

void stopPresentation()
{
    free(
        frequencies
    );

    UnloadRenderTexture(
        renderTarget
    );

    CloseWindow();
}
