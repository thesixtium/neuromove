/**
 * @file run_unicorn.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Minimal TinyBCI runner using a Unicorn producer.
 *
 * Demonstrates the full pipeline:
 *   UnicornProducer → TBCI_Input → SegmentationNode → TBCI_EpochQueue
 *
 * Epochs are printed to stdout as they arrive. Replace the synthetic
 * producer with an LSL producer, file reader, or hardware driver
 * to run with real data.
 *
 * ## Build
 *
 *   cmake --build . --target tinybci_runner
 *
 * ## Usage
 *
 *   ./bin/tinybci_runner [n_epochs]
 *
 *   n_epochs — number of epochs to collect before stopping (default: 10)
 */
#include "nodes/decoder/tbci_cca_model.h"
#include "nodes/features/tbci_cca_node.h"
#if defined(__linux__)
#   define _POSIX_C_SOURCE 199309L
#endif

#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include "tbci_context.h"
#include "../include/mathutils/tbci_sizeof.h"
#include "../producer/tbci_producer_factory.h"
#include "nodes/preprocessing/tbci_bandpass_node.h"
#include "nodes/preprocessing/tbci_notch_node.h"

/* --------------------------------------------------------------------------
 * Capacity constants
 * -------------------------------------------------------------------------- */

#define N_CHANNELS        8
#define SIG_CAPACITY    1024    /* samples — must fit pre+post window */
#define TRIG_CAPACITY     32
#define EPOCH_CAPACITY     8
#define SRATE          250.0f
#define PRE_MS           200
#define POST_MS          800
#define TOTAL_FRAMES     250    /* (PRE_MS + POST_MS) / 1000 * SRATE */
#define TRIGGER_CODE       1u
#define TRIGGER_INTERVAL_MS 1000u  /* fire a trigger every second */
#define PORT "/dev/cu.UN-20230805"

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
static TBCI_Epoch    output_storage  [EPOCH_CAPACITY];
static float         output_pool     [EPOCH_CAPACITY * TOTAL_FRAMES * N_CHANNELS];

static TBCI_SignalBuffer  signal_buf;
static TBCI_SignalBuffer  processed_buf;
static TBCI_TriggerQueue  trigger_queue;
static TBCI_EpochQueue    epoch_queue;
static TBCI_EpochQueue    features_queue;
static TBCI_EpochQueue    output_queue;
static TBCI_Input         inputs;
static TBCI_Config        config;
static TBCI_Context       ctx;

static UnicornProducerConfig producer_config;
static UnicornProducer       prod;

static TBCI_TriggerGeneratorConfig tg_config;
static TBCI_TriggerGeneratorState  tg_state;
static TBCI_TriggerGenerator       tg;

TBCI_Producer *producer = (TBCI_Producer *)&prod;

// CCA constants
#define N_FREQS         6
#define N_HARMONICS     2
#define N_COMPONENTS    (N_HARMONICS * 2)
#define REF_CAP         (N_FREQS * N_COMPONENTS * TOTAL_FRAMES)
static float ref_signals[REF_CAP];

// Nodes
static TBCI_NotchNode notch_node;
static TBCI_NotchConfig notch_config;
static TBCI_BandpassNode  bp_node;
static TBCI_BandpassConfig bp_config;
static TBCI_CCANode cca_node;
static TBCI_CCAConfig cca_config;
static TBCI_CCAModel cca_model;
static TBCI_CCAModelConfig cca_model_config;

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
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000u + (uint64_t)ts.tv_nsec / 1000u;
}

static void print_epoch(const TBCI_Epoch *epoch, size_t index)
{
    printf("[Epoch %zu] label=%u encoded=%u predicted=%u timestamp=%.3f s  frames=%zu  channels=%zu\n",
           index,
           epoch->label,
           epoch->encoded_label,
           epoch->predicted_label,
           (double)epoch->timestamp_us / 1e6,
           epoch->n_frames,
           epoch->n_channels);
}

static int init_pipeline(bool sliding, const char *port)
{
    /* buffers */
    sb_init(&signal_buf, sig_storage, sig_timestamps, sig_indices,SIG_CAPACITY, N_CHANNELS);
    sb_init(&processed_buf, proc_storage, proc_timestamps, proc_indices,SIG_CAPACITY, N_CHANNELS);
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
    config.mode              = sliding ? SEG_MODE_SLIDING : SEG_MODE_TRIGGERED;
    config.pre_stimulus_ms   = PRE_MS;
    config.post_stimulus_ms  = POST_MS;
    config.overlap_ms        = sliding ? 400u : 0u;
    config.trial_end_code    = sliding ? 10u  : 0u;
    config.use_preprocessing      = true;
    config.use_feature_extraction = true;
    config.use_decoder            = true;
    config.log_enabled            = true;  /* set true to enable CSV logging */
    config.log_processed          = false;  /* set true to save preprocessed data instead of raw data to CSV */
    config.log_subject[0]         = '\0';
    strncpy(config.log_session, "unicorn", sizeof(config.log_session) - 1);
    config.log_session[sizeof(config.log_session) - 1] = '\0';  /* ensure null termination */

    TBCI_Status s;

    /* register notch & bandpass node in preprocessing group */
    notch_config.freq_hz = 60.0f;
    notch_config.q_factor   = 10.0f;
    notch_config.n_harmonics = 2;
    s = notch_init(&notch_node, &notch_config);
    if (s != TBCI_OK) {
        fprintf(stderr, "notch_init failed: %d\n", s);
        return s;
    }

    bp_configure(&bp_config, 1.0f, 40.0f, 3);  /* 6th order Butterworth */
    s = bp_init(&bp_node, &bp_config);
    if (s != TBCI_OK) {
        fprintf(stderr, "bp_init failed: %d\n", s);
        return s;
    }

    /* Register CCA node and model */
    cca_config.n_freqs = N_FREQS;
    cca_config.n_harmonics = N_HARMONICS;
    cca_config.freqs[0]    = 7.0f;
    cca_config.freqs[4]    = 7.5f;
    cca_config.freqs[1]    = 8.0f;
    cca_config.freqs[5]    = 8.5f;
    cca_config.freqs[2]    = 9.0f;
    cca_config.freqs[3]    = 11.0f;

    s = cca_init(&cca_node, &cca_config, ref_signals, REF_CAP);
    if (s != TBCI_OK) {
        fprintf(stderr, "cca_init failed: %d\n", s);
        return s;
    }

    cca_model_config.temperature = 0.3f;
    cca_model_config.n_freqs = N_FREQS;
    cca_model_init(&cca_model, &cca_model_config);

    group_add_node(&ctx.preprocessing.group, (TBCI_Node *)&notch_node);
    group_add_node(&ctx.preprocessing.group, (TBCI_Node *)&bp_node);
    group_add_node(&ctx.features.group, (TBCI_Node *)&cca_node);
    group_add_node(&ctx.decoder.group, (TBCI_Node *)&cca_model);

    s = tbci_context_init(&ctx, &config, &inputs, &processed_buf, &epoch_queue, &features_queue, &output_queue);
    if (s != TBCI_OK) return s;

    producer_config.n_channels = N_CHANNELS;
    producer_config.srate = SRATE;
    if (port == NULL)
        producer_config.port = PORT;
    else
        producer_config.port = port;

    tbci_producer_create_unicorn(producer, &producer_config, &inputs, &ctx);

    // Initialize trigger generator
    tg_config.trigger_interval_ms = sliding ? 6000u : 1000u;  // was 2000u
    tg_config.trigger_code        = 1u;
    tg_config.trial_end_code      = sliding ? 10u   : 0u;
    tg_config.trial_duration_ms   = sliding ? 5000u : 0u;

    tg_init(&tg, &tg_config, &tg_state);
    prod.trigger_gen = &tg;

    return TBCI_OK;
}

static void* input_thread(void *arg)
{
    TBCI_Input *inputs = (TBCI_Input *)arg;
    int c;
    while ((c = getchar()) != EOF) {
        TBCI_Trigger cmd = {.timestamp_us = now_us()};
        switch (c) {
        case 'i': cmd.code = 193u; break;  // inference
        case 't': cmd.code = 194u; break;  // training
        case 's': cmd.code = 192u; break;  // stop
        default: continue;
        }
        in_push_trigger(inputs, &cmd, &ctx);
        printf("Command sent: %c (code=%u)\n", c, cmd.code);
    }
    return NULL;
}

/* --------------------------------------------------------------------------
 * Main
 * -------------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
    TBCI_SizeofReport report = {
        .sig_storage_bytes   = sizeof(sig_storage),
        .proc_storage_bytes  = sizeof(proc_storage),
        .epoch_pool_bytes    = sizeof(epoch_pool),
        .features_pool_bytes = sizeof(features_pool),
        .output_pool_bytes   = sizeof(output_pool),
        .train_trials_bytes  = sizeof(0),
        .train_labels_bytes  = sizeof(0),
        .train_capacity      = 0,
        .input_size          = TOTAL_FRAMES * N_CHANNELS,
    };

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    size_t target_epochs = 10;
    bool sliding = false;
    const char *port = "/dev/cu.UN-20230805";

    if (argc > 1) target_epochs = (size_t)atoi(argv[1]);
    if (argc > 2) sliding       = (strcmp(argv[2], "sliding") == 0);
    if (argc > 3) port = argv[3];

    printf("TinyBCI Runner\n");
    printf("  channels=%d  srate=%.0f Hz  window=%d ms  mode=%s  target=%zu epochs\n\n",
           N_CHANNELS, SRATE, PRE_MS + POST_MS,
           sliding ? "sliding" : "triggered", target_epochs);
    printf("Available commands: s=Stop, t=Start Training, i=Start Inference\n");

    if (init_pipeline(sliding, port) !=  TBCI_OK)
    {
        fprintf(stderr, "Failed to initialise pipeline.\n");
        return TBCI_ERR_INVALID_STATE;
    }

    tbci_print_sizeof(&ctx, &report);

    if (tbci_context_start(&ctx, TBCI_STATE_INFERENCE) != TBCI_OK) {
        fprintf(stderr, "Failed to start TinyBCI pipeline.\n");
        return 1;
    }

    printf("Running — press Ctrl+C to stop\n\n");
    printf("Pipeline running. Reading EEG data...\n");

    size_t epochs_collected = 0;
    bool endless = (target_epochs == 0);
    uint64_t next_tick_us = now_us();


    pthread_t input_tid;
    pthread_create(&input_tid, NULL, input_thread, &inputs);

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

    printf("\nDone. Collected %zu / %zu epochs.\n", epochs_collected, target_epochs);
    return (epochs_collected == target_epochs) ? EXIT_SUCCESS : EXIT_FAILURE;
}
