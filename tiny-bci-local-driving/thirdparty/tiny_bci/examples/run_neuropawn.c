/**
 * @file run_neuropawn.c
 *
 * @brief Minimal TinyBCI runner using a NeuroPawn Knight producer.
 *
 * Demonstrates the full SSVEP/CCA pipeline driven by real hardware:
 *   NeuroPawnProducer → TBCI_Input → SegmentationNode → TBCI_EpochQueue
 *
 * The producer opens the serial port, runs the Knight start-up command
 * sequence (channel enable + optional right-leg-drive), auto-detects the board
 * type (non-IMU vs IMU), and streams 8 EXG channels into the pipeline.
 *
 * ## Build
 *
 *   cmake --build . --target tinybci_neuropawn_runner
 *
 * ## Usage
 *
 *   ./bin/tinybci_neuropawn_runner [n_epochs] [mode] [port]
 *
 *   n_epochs — epochs to collect before stopping (0 = run until Ctrl+C). Default 0.
 *   mode     — "triggered" or "sliding" (SSVEP). Default "sliding".
 *   port     — serial port. Default "COM3" (Windows) / "/dev/ttyUSB0" (POSIX).
 */

#include <stdio.h>
#include <signal.h>
#include <stdint.h>
#include <stdbool.h>

#include "tbci_context.h"
#include "../producer/neuropawn_producer.h"
#include "mathutils/tbci_sizeof.h"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <time.h>
#endif

/* --------------------------------------------------------------------------
 * Pipeline configuration
 * -------------------------------------------------------------------------- */

#define N_CHANNELS        8
#define SIG_CAPACITY    1024     /* samples — must fit pre+post window */
#define TRIG_CAPACITY     32
#define EPOCH_CAPACITY     8
#define SRATE          125.0f    /* NeuroPawn Knight sampling rate */
#define PRE_MS           200
#define POST_MS          800
#define TOTAL_FRAMES     125     /* (PRE_MS + POST_MS) / 1000 * SRATE */

#if defined(_WIN32) || defined(_WIN64)
#define DEFAULT_PORT "COM3"
#else
#define DEFAULT_PORT "/dev/ttyUSB0"
#endif

/* --------------------------------------------------------------------------
 * Static storage — all caller-provided, no malloc
 * -------------------------------------------------------------------------- */

static float    sig_storage   [SIG_CAPACITY   * N_CHANNELS];
static uint64_t sig_timestamps[SIG_CAPACITY];
static uint32_t sig_indices   [SIG_CAPACITY];
static float    proc_storage   [SIG_CAPACITY * N_CHANNELS];
static uint64_t proc_timestamps[SIG_CAPACITY];
static uint32_t proc_indices   [SIG_CAPACITY];
static TBCI_Trigger  trig_storage  [TRIG_CAPACITY];
static TBCI_Epoch    epoch_storage [EPOCH_CAPACITY];
static float    epoch_pool    [EPOCH_CAPACITY * TOTAL_FRAMES * N_CHANNELS];
static TBCI_Epoch    features_storage [EPOCH_CAPACITY];
static float    features_pool    [EPOCH_CAPACITY * TOTAL_FRAMES * N_CHANNELS];
static TBCI_Epoch    output_storage   [EPOCH_CAPACITY];
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

static NeuroPawnProducerConfig producer_config;
static NeuroPawnProducer       prod;

static TBCI_TriggerGeneratorConfig tg_config;
static TBCI_TriggerGeneratorState  tg_state;
static TBCI_TriggerGenerator       tg;

static TBCI_Producer *producer = (TBCI_Producer *)&prod;

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */
static volatile bool running = true;

static void handle_signal(int sig)
{
    (void)sig;
    running = false;
}

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

static void print_epoch(const TBCI_Epoch *epoch, size_t index)
{
    printf("[Epoch %zu] label=%u  timestamp=%.3f s  frames=%zu  channels=%zu\n",
           index,
           epoch->label,
           (double)epoch->timestamp_us / 1e6,
           epoch->n_frames,
           epoch->n_channels);
}

static void init_pipeline(bool sliding, const char *port)
{
    /* buffers */
    sb_init(&signal_buf, sig_storage, sig_timestamps, sig_indices,
            SIG_CAPACITY, N_CHANNELS);
    sb_init(&processed_buf, proc_storage, proc_timestamps, proc_indices,
            SIG_CAPACITY, N_CHANNELS);
    tq_init(&trigger_queue, trig_storage, TRIG_CAPACITY);
    eq_init(&epoch_queue, epoch_storage, EPOCH_CAPACITY, TOTAL_FRAMES);
    eq_configure(&epoch_queue, epoch_pool, N_CHANNELS);
    eq_init(&features_queue, features_storage, EPOCH_CAPACITY, TOTAL_FRAMES);
    eq_configure(&features_queue, features_pool, N_CHANNELS);
    eq_init(&output_queue, output_storage, EPOCH_CAPACITY, TOTAL_FRAMES);
    eq_configure(&output_queue, output_pool, N_CHANNELS);

    inputs.signal     = &signal_buf;
    inputs.triggers   = &trigger_queue;
    inputs.n_channels = N_CHANNELS;

    config.paradigm          = sliding ? TBCI_PARADIGM_MI : TBCI_PARADIGM_P300;
    config.nominal_srate     = SRATE;
    config.target_srate      = SRATE;
    config.n_channels        = N_CHANNELS;
    config.window_length_ms  = PRE_MS + POST_MS;
    config.use_preprocessing      = false;
    config.use_feature_extraction = false;
    config.mode              = sliding ? SEG_MODE_SLIDING : SEG_MODE_TRIGGERED;
    config.pre_stimulus_ms   = PRE_MS;
    config.post_stimulus_ms  = POST_MS;
    config.overlap_ms        = sliding ? 400u : 0u;
    config.target_code       = 1;
    config.trial_end_code    = sliding ? 0u  : 10u;

    tbci_context_init(&ctx, &config, &inputs, &processed_buf, &epoch_queue, &features_queue, &output_queue);

    /* NeuroPawn producer configuration */
    producer_config.port         = port;
    producer_config.srate        = SRATE;
    producer_config.n_channels   = N_CHANNELS;
    producer_config.gain         = NEUROPAWN_DEFAULT_GAIN;
    producer_config.cmd_pause_ms = NEUROPAWN_CMD_PAUSE_MS;
    for (int i = 0; i < N_CHANNELS; i++) {
        producer_config.channel_enabled[i] = true;   /* enable all channels */
        producer_config.rld_enabled[i]     = false;  /* no right-leg-drive  */
    }

    /* synthetic trigger generator — provides timed triggers for SSVEP */
    tg_config.trigger_interval_ms = sliding ? 2000u : 1000u;
    tg_config.trigger_code        = 1u;
    tg_config.trial_end_code      = sliding ? 10u   : 0u;
    tg_config.trial_duration_ms   = sliding ? 3000u : 0u;

    tg_init(&tg, &tg_config, &tg_state);
    prod.trigger_gen = &tg;

     /* initialise the producer and run the Knight start-up sequence */
    np_init(&prod, &producer_config);
    TBCI_Status s = producer->init(producer, &inputs, &ctx);
    if (s != TBCI_OK) {
        fprintf(stderr, "neuropawn: failed to connect on %s (status=%d)\n", port, s);
        exit(EXIT_FAILURE);
    }
}

/* --------------------------------------------------------------------------
 * Main
 * -------------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
    /*tbci_print_sizeof();*/
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    size_t      target_epochs = 0;        /* 0 = run until Ctrl+C */
    bool        sliding       = true;     /* SSVEP default */
    const char *port          = DEFAULT_PORT;

    if (argc > 1) target_epochs = (size_t)atoi(argv[1]);
    if (argc > 2) sliding       = (strcmp(argv[2], "sliding") == 0);
    if (argc > 3) port          = argv[3];

    printf("TinyBCI NeuroPawn Runner\n");
    printf("  channels=%d  srate=%.0f Hz  window=%d ms  mode=%s  port=%s  target=%zu epochs\n\n",
           N_CHANNELS, SRATE, PRE_MS + POST_MS,
           sliding ? "sliding" : "triggered", port, target_epochs);

    init_pipeline(sliding, port);
    tbci_context_start(&ctx, TBCI_STATE_INFERENCE);

    size_t   epochs_collected = 0;
    bool     endless          = (target_epochs == 0);
    uint64_t next_tick_us     = now_us();

    printf("Running — press Ctrl+C to stop\n\n");

    while ((endless || epochs_collected < target_epochs) && running)
    {
        uint64_t now = now_us();

        if (now >= next_tick_us)
        {
            /* produce one sample */
            TBCI_Status tick_status = producer->tick(producer, &inputs, &ctx);
            if (tick_status != TBCI_OK && tick_status != TBCI_ERR_EMPTY) {
                fprintf(stderr, "Producer error\n");
                break;
            }

            /* process pipeline */
            TBCI_Status status = tbci_context_tick(&ctx);
            if (status != TBCI_OK) {
                fprintf(stderr, "Pipeline error\n");
                break;
            }

            /* consume epochs */
            while (!eq_is_empty(&output_queue)) {
                TBCI_Epoch epoch;
                eq_pop(&output_queue, &epoch);
                print_epoch(&epoch, epochs_collected++);
            }

            /* schedule next tick */
            uint32_t spacing_us = (uint32_t)(1000000.0f / SRATE);
            next_tick_us += spacing_us;
        }
    }

    producer->close(producer);
    tbci_context_stop(&ctx);

    printf("\nDone. Collected %zu epochs.\n", epochs_collected);
    return EXIT_SUCCESS;
}
