/**
 * @file motor_imagery_neuropawn.c
 *
 * @brief 4-class Motor Imagery BCI runner — NeuroPawn Knight + ONNX EEGNet.
 *
 * Connects to the NeuroPawn Knight EEG headset, applies an 8–30 Hz bandpass
 * filter and 50 Hz notch filter, segments 1-second triggered epochs, and
 * classifies them with an EEGNet model exported from the Python training
 * pipeline.
 *
 * Classes:  0 = left hand   1 = right hand   2 = rest   3 = both hands
 *
 * ## Build (requires -DTBCI_WITH_ONNX=ON)
 *
 *   cmake -B build -DTBCI_WITH_ONNX=ON
 *   cmake --build build --target tinybci_mi_neuropawn_runner
 *
 * ## Usage
 *
 *   ./bin/tinybci_mi_neuropawn_runner [port] [model.onnx]
 *
 *   port       — serial port.  Default: COM3 (Windows) / /dev/ttyUSB0 (POSIX)
 *   model.onnx — path to the exported EEGNet model.  Default: mi_motor_imagery.onnx
 *
 * ## Electrode placement
 *
 *   Use the same 8-channel sensorimotor montage as data collection:
 *   FC4, C4, CP4, C2, C1, CP3, C3, FC3  (left→right across motor cortex)
 *
 * ## Timing
 *
 *   A synthetic trigger fires every TRIAL_INTERVAL_MS milliseconds.
 *   When you see ">>> Imagine: <CUE>" focus on that mental action for 1 second.
 *   The predicted class is printed immediately after classification.
 */

#ifdef TBCI_WITH_ONNX

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "tbci_context.h"
#include "nodes/preprocessing/tbci_bandpass_node.h"
#include "nodes/preprocessing/tbci_notch_node.h"
#include "nodes/decoder/tbci_onnx_model.h"
#include "../producer/neuropawn_producer.h"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <time.h>
#endif

/* --------------------------------------------------------------------------
 * Pipeline configuration
 * -------------------------------------------------------------------------- */

#define N_CHANNELS          8
#define SIG_CAPACITY     1024     /* samples — must fit window */
#define TRIG_CAPACITY      32
#define EPOCH_CAPACITY      8
#define SRATE           125.0f    /* NeuroPawn Knight sampling rate (Hz) */
#define PRE_MS              0     /* no pre-stimulus baseline */
#define POST_MS          1000     /* 1-second imagery window */
#define TOTAL_FRAMES      125     /* POST_MS / 1000 * SRATE */
#define TRIAL_INTERVAL_MS 4000u   /* cue every 4 s — gives time to prepare */

#if defined(_WIN32) || defined(_WIN64)
#define DEFAULT_PORT "COM3"
#else
#define DEFAULT_PORT "/dev/ttyUSB0"
#endif

#define DEFAULT_MODEL "mi_motor_imagery.onnx"

/* Class names matching the Python training labels */
static const char *CLASS_NAMES[4] = { "LEFT", "RIGHT", "REST", "BOTH" };

/* --------------------------------------------------------------------------
 * Static storage — no malloc
 * -------------------------------------------------------------------------- */

static float    sig_storage      [SIG_CAPACITY * N_CHANNELS];
static uint64_t sig_timestamps   [SIG_CAPACITY];
static uint32_t sig_indices      [SIG_CAPACITY];
static float    proc_storage     [SIG_CAPACITY * N_CHANNELS];
static uint64_t proc_timestamps  [SIG_CAPACITY];
static uint32_t proc_indices     [SIG_CAPACITY];
static TBCI_Trigger  trig_storage   [TRIG_CAPACITY];
static TBCI_Epoch    epoch_storage  [EPOCH_CAPACITY];
static float    epoch_pool       [EPOCH_CAPACITY * TOTAL_FRAMES * N_CHANNELS];
static TBCI_Epoch    features_storage[EPOCH_CAPACITY];
static float    features_pool    [EPOCH_CAPACITY * TOTAL_FRAMES * N_CHANNELS];
static TBCI_Epoch    output_storage [EPOCH_CAPACITY];
static float    output_pool      [EPOCH_CAPACITY * TOTAL_FRAMES * N_CHANNELS];

static TBCI_SignalBuffer  signal_buf;
static TBCI_SignalBuffer  processed_buf;
static TBCI_TriggerQueue  trigger_queue;
static TBCI_EpochQueue    epoch_queue;
static TBCI_EpochQueue    features_queue;
static TBCI_EpochQueue    output_queue;
static TBCI_Input         inputs;
static TBCI_Config        config;
static TBCI_Context       ctx;

/* Preprocessing nodes */
static TBCI_NotchNode      notch_node;
static TBCI_NotchConfig    notch_config;
static TBCI_BandpassNode   bp_node;
static TBCI_BandpassConfig bp_config;

/* ONNX decoder */
static TBCI_ONNXModel       onnx_model;
static TBCI_ONNXModelConfig onnx_config;

/* NeuroPawn producer */
static NeuroPawnProducerConfig producer_config;
static NeuroPawnProducer       prod;
static TBCI_TriggerGeneratorConfig tg_config;
static TBCI_TriggerGeneratorState  tg_state;
static TBCI_TriggerGenerator       tg;
static TBCI_Producer *producer = (TBCI_Producer *)&prod;

/* Cue cycling — rotate through classes for display purposes */
static size_t cue_index = 0;
static const char *CUE_SEQUENCE[4] = { "LEFT hand", "RIGHT hand", "BOTH hands", "REST" };

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

static volatile bool running = true;

static void handle_signal(int sig) { (void)sig; running = false; }

static uint64_t now_us(void)
{
#if defined(_WIN32) || defined(_WIN64)
    static LARGE_INTEGER freq;
    LARGE_INTEGER counter;
    if (freq.QuadPart == 0)
        QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (uint64_t)(counter.QuadPart * 1000000ull / (uint64_t)freq.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000u + (uint64_t)ts.tv_nsec / 1000u;
#endif
}

/* --------------------------------------------------------------------------
 * Pipeline initialisation
 * -------------------------------------------------------------------------- */

static void init_pipeline(const char *port, const char *model_path)
{
    /* Signal and trigger buffers */
    sb_init(&signal_buf,    sig_storage,  sig_timestamps,  sig_indices,
            SIG_CAPACITY, N_CHANNELS);
    sb_init(&processed_buf, proc_storage, proc_timestamps, proc_indices,
            SIG_CAPACITY, N_CHANNELS);
    tq_init(&trigger_queue, trig_storage, TRIG_CAPACITY);

    /* Epoch queues */
    eq_init(&epoch_queue,    epoch_storage,    EPOCH_CAPACITY, TOTAL_FRAMES);
    eq_configure(&epoch_queue, epoch_pool, N_CHANNELS);
    eq_init(&features_queue, features_storage, EPOCH_CAPACITY, TOTAL_FRAMES);
    eq_configure(&features_queue, features_pool, N_CHANNELS);
    eq_init(&output_queue,   output_storage,   EPOCH_CAPACITY, TOTAL_FRAMES);
    eq_configure(&output_queue, output_pool, N_CHANNELS);

    inputs.signal     = &signal_buf;
    inputs.triggers   = &trigger_queue;
    inputs.n_channels = N_CHANNELS;

    /* Pipeline config — triggered mode, 1-second epoch
     * NOTE: triggered segmentation is associated with TBCI_PARADIGM_P300
     * in the framework's consistency check. The paradigm label does not
     * affect actual behaviour — classification is determined by the ONNX model. */
    config.paradigm          = TBCI_PARADIGM_P300;
    config.nominal_srate     = SRATE;
    config.target_srate      = SRATE;
    config.n_channels        = N_CHANNELS;
    config.window_length_ms  = PRE_MS + POST_MS;
    config.mode              = SEG_MODE_TRIGGERED;
    config.pre_stimulus_ms   = PRE_MS;
    config.post_stimulus_ms  = POST_MS;
    config.overlap_ms        = 0;
    config.trial_end_code    = 0;
    config.use_preprocessing      = true;   /* 8-30 Hz bandpass + 50 Hz notch */
    config.use_feature_extraction = false;  /* EEGNet takes raw filtered epoch */
    config.use_decoder            = true;   /* ONNX inference */

    /* 50 Hz notch — removes power-line noise */
    notch_config.freq_hz     = 50.0f;
    notch_config.q_factor    = 30.0f;
    notch_config.n_harmonics = 1;
    if (notch_init(&notch_node, &notch_config) != TBCI_OK) {
        fprintf(stderr, "notch_init failed\n");
        exit(EXIT_FAILURE);
    }

    /* 8-30 Hz bandpass — motor imagery frequency band */
    bp_config.low_hz  = 8.0f;
    bp_config.high_hz = 30.0f;
    if (bp_init(&bp_node, &bp_config) != TBCI_OK) {
        fprintf(stderr, "bp_init failed\n");
        exit(EXIT_FAILURE);
    }

    /* Register preprocessing nodes (notch first, then bandpass) */
    group_add_node(&ctx.preprocessing.group, (TBCI_Node *)&notch_node);
    group_add_node(&ctx.preprocessing.group, (TBCI_Node *)&bp_node);

    /* ONNX model config */
    strncpy(onnx_config.model_path, model_path, TBCI_ONNX_MAX_PATH_LEN - 1);
    onnx_config.train_trials        = NULL;   /* inference only — no training */
    onnx_config.train_labels        = NULL;
    onnx_config.train_capacity      = 0;
    onnx_config.n_folds             = 0;
    onnx_config.output_mode         = TBCI_OUTPUT_SOFTMAX;
    onnx_config.scorer              = NULL;
    onnx_config.sigmoid_threshold   = 0.5f;
    /* input_name / output_name default to "input" / "output" — matches export */

    /* Register ONNX model in decoder group */
    group_add_node(&ctx.decoder.group, (TBCI_Node *)&onnx_model);

    /* Initialise context (must happen before onnx_model_init)
     * Use s < TBCI_OK to ignore paradigm-mode mismatch warnings (status > 0) */
    TBCI_Status s = tbci_context_init(&ctx, &config, &inputs, &processed_buf,
                                      &epoch_queue, &features_queue, &output_queue);
    if (s < TBCI_OK) {
        fprintf(stderr, "tbci_context_init failed (status=%d)\n", s);
        exit(EXIT_FAILURE);
    }

    /* Initialise ONNX model (opens the .onnx file, reads tensor dims) */
    s = onnx_model_init(&onnx_model, &onnx_config, &ctx);
    if (s != TBCI_OK) {
        fprintf(stderr, "onnx_model_init failed (status=%d)\n  "
                "Ensure '%s' is present next to the binary.\n", s, model_path);
        exit(EXIT_FAILURE);
    }

    /* NeuroPawn producer */
    producer_config.port         = port;
    producer_config.srate        = SRATE;
    producer_config.n_channels   = N_CHANNELS;
    producer_config.gain         = NEUROPAWN_DEFAULT_GAIN;
    producer_config.cmd_pause_ms = NEUROPAWN_CMD_PAUSE_MS;
    for (int i = 0; i < N_CHANNELS; i++) {
        producer_config.channel_enabled[i] = true;
        producer_config.rld_enabled[i]     = false;
    }

    /* Timed trigger — fires every TRIAL_INTERVAL_MS */
    tg_config.trigger_interval_ms = TRIAL_INTERVAL_MS;
    tg_config.trigger_code        = 1u;
    tg_config.trial_end_code      = 0u;
    tg_config.trial_duration_ms   = 0u;
    tg_init(&tg, &tg_config, &tg_state);
    prod.trigger_gen = &tg;

    np_init(&prod, &producer_config);
    s = producer->init(producer, &inputs, &ctx);
    if (s != TBCI_OK) {
        fprintf(stderr, "neuropawn: failed to connect on %s (status=%d)\n", port, s);
        exit(EXIT_FAILURE);
    }
}

/* --------------------------------------------------------------------------
 * Main loop
 * -------------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
    signal(SIGINT,  handle_signal);
    signal(SIGTERM, handle_signal);

    const char *port       = (argc > 1) ? argv[1] : DEFAULT_PORT;
    const char *model_path = (argc > 2) ? argv[2] : DEFAULT_MODEL;

    printf("+=======================================================+\n");
    printf("|  TinyBCI -- Motor Imagery Runner (NeuroPawn + ONNX)  |\n");
    printf("+=======================================================+\n\n");
    printf("  Port     : %s\n", port);
    printf("  Model    : %s\n", model_path);
    printf("  Srate    : %.0f Hz\n", SRATE);
    printf("  Window   : %d ms\n", POST_MS);
    printf("  Classes  : left / right / rest / both\n");
    printf("  Filter   : 8-30 Hz bandpass + 50 Hz notch\n");
    printf("  Trial    : every %.1f s\n\n", TRIAL_INTERVAL_MS / 1000.0);
    printf("Electrode order (pin 1→8): FC4, C4, CP4, C2, C1, CP3, C3, FC3\n\n");

    printf("Connecting to NeuroPawn …\n");
    init_pipeline(port, model_path);
    tbci_context_start(&ctx, TBCI_STATE_INFERENCE);

    printf("Running — press Ctrl+C to stop\n\n");
    printf("A cue will appear every %.1f seconds.\n"
           "Focus on the suggested mental action for 1 second after the cue.\n\n",
           TRIAL_INTERVAL_MS / 1000.0);

    size_t   epochs_classified = 0;
    uint64_t next_tick_us      = now_us();
    uint64_t next_cue_us       = now_us() + (uint64_t)TRIAL_INTERVAL_MS * 1000u;

    while (running)
    {
        uint64_t now = now_us();

        /* Print the next cue just before the trigger fires */
        if (now >= next_cue_us) {
            printf("---------------------------------------------\n");
            printf(">>> Imagine: %s\n", CUE_SEQUENCE[cue_index % 4]);
            cue_index++;
            next_cue_us += (uint64_t)TRIAL_INTERVAL_MS * 1000u;
        }

        /* Tick at hardware rate */
        if (now >= next_tick_us) {
            TBCI_Status tick_s = producer->tick(producer, &inputs, &ctx);
            if (tick_s != TBCI_OK && tick_s != TBCI_ERR_EMPTY) {
                fprintf(stderr, "Producer error (status=%d)\n", tick_s);
                break;
            }

            TBCI_Status pipe_s = tbci_context_tick(&ctx);
            if (pipe_s != TBCI_OK) {
                fprintf(stderr, "Pipeline error (status=%d)\n", pipe_s);
                break;
            }

            /* Consume classified epochs */
            while (!eq_is_empty(&output_queue)) {
                TBCI_Epoch epoch;
                eq_pop(&output_queue, &epoch);

                int   pred       = onnx_model.base_model.predicted_class;
                float confidence = onnx_model.base_model.confidence;

                const char *label = (pred >= 0 && pred < 4) ? CLASS_NAMES[pred] : "?";

                printf("  [Epoch %3zu]  predicted = %-6s  confidence = %.2f\n",
                       epochs_classified++, label, (double)confidence);
            }

            uint32_t spacing_us = (uint32_t)(1000000.0f / SRATE);
            next_tick_us += spacing_us;
        }
    }

    producer->close(producer);
    tbci_context_stop(&ctx);

    printf("\nDone. Classified %zu epochs.\n", epochs_classified);
    return EXIT_SUCCESS;
}

#else /* TBCI_WITH_ONNX not defined */

#include <stdio.h>
int main(void)
{
    fprintf(stderr,
        "This runner requires ONNX Runtime support.\n"
        "Rebuild with:  cmake -B build -DTBCI_WITH_ONNX=ON\n"
        "               cmake --build build --target tinybci_mi_neuropawn_runner\n");
    return 1;
}

#endif /* TBCI_WITH_ONNX */
