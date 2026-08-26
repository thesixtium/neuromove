/**
 * @file run_onnx.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief TinyBCI runner using a synthetic producer and ONNX model for P300.
 *
 * Demonstrates the full pipeline with ONNX inference:
 *   SyntheticProducer → Preprocessing → CoreNode → FeatureExtraction → ONNX Decoder
 *
 * ## Build
 *
 *   cmake --build . --target tinybci_onnx_runner
 *
 * ## Usage
 *
 *   ./bin/tinybci_onnx_runner [n_epochs]
 *
 *   n_epochs — number of epochs to collect before stopping (default: 10)
 */

#if defined(__linux__)
#   define _POSIX_C_SOURCE 199309L
#endif

#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include "tbci_context.h"
#include "../include/mathutils/tbci_sizeof.h"
#include "../producer/tbci_producer_factory.h"
#include "../producer/synthetic_producer.h"
#include "nodes/preprocessing/tbci_bandpass_node.h"
#include "nodes/preprocessing/tbci_notch_node.h"
#include "nodes/decoder/tbci_onnx_model.h"

/* --------------------------------------------------------------------------
 * Pipeline configuration
 * -------------------------------------------------------------------------- */

#define N_CHANNELS       8
#define SIG_CAPACITY     1024
#define TRIG_CAPACITY    32
#define EPOCH_CAPACITY   8
#define SRATE            250.0f
#define PRE_MS           100
#define POST_MS          700
#define TOTAL_FRAMES     ((size_t)((PRE_MS + POST_MS) * SRATE / 1000.0f))
#define MAX_TRAIN_TRIALS 60

/* --------------------------------------------------------------------------
 * Static storage — no malloc
 * -------------------------------------------------------------------------- */

static float         sig_storage     [SIG_CAPACITY * N_CHANNELS];
static uint64_t      sig_timestamps  [SIG_CAPACITY];
static uint32_t      sig_indices     [SIG_CAPACITY];
static float         proc_storage    [SIG_CAPACITY * N_CHANNELS];
static uint64_t      proc_timestamps [SIG_CAPACITY];
static uint32_t      proc_indices    [SIG_CAPACITY];
static TBCI_Trigger  trig_storage    [TRIG_CAPACITY];
static TBCI_Epoch    epoch_storage   [EPOCH_CAPACITY];
static float         epoch_pool      [EPOCH_CAPACITY * TOTAL_FRAMES * N_CHANNELS];
static TBCI_Epoch    features_storage[EPOCH_CAPACITY];
static float         features_pool   [EPOCH_CAPACITY * TOTAL_FRAMES * N_CHANNELS];
static TBCI_Epoch    output_storage  [EPOCH_CAPACITY];
static float         output_pool     [EPOCH_CAPACITY * TOTAL_FRAMES * N_CHANNELS];

/* ONNX calibration storage */
static float         train_trials    [MAX_TRAIN_TRIALS * N_CHANNELS * TOTAL_FRAMES];
static uint16_t      train_labels    [MAX_TRAIN_TRIALS];

static TBCI_SignalBuffer  signal_buf;
static TBCI_SignalBuffer  processed_buf;
static TBCI_TriggerQueue  trigger_queue;
static TBCI_EpochQueue    epoch_queue;
static TBCI_EpochQueue    features_queue;
static TBCI_EpochQueue    output_queue;
static TBCI_Input         inputs;
static TBCI_Config        config;
static TBCI_Context       ctx;

static SyntheticProducerConfig producer_config;
static SyntheticProducer       prod;

static TBCI_TriggerGeneratorConfig tg_config;
static TBCI_TriggerGeneratorState  tg_state;
static TBCI_TriggerGenerator       tg;

TBCI_Producer *producer = (TBCI_Producer *)&prod;

/* Nodes */
static TBCI_NotchNode      notch_node;
static TBCI_NotchConfig    notch_config;
static TBCI_BandpassNode   bp_node;
static TBCI_BandpassConfig bp_config;
static TBCI_ONNXModel      onnx_model;
static TBCI_ONNXModelConfig onnx_config;

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

static int init_pipeline(bool sliding)
{
    sb_init(&signal_buf,    sig_storage,  sig_timestamps,  sig_indices,  SIG_CAPACITY, N_CHANNELS);
    sb_init(&processed_buf, proc_storage, proc_timestamps, proc_indices, SIG_CAPACITY, N_CHANNELS);
    tq_init(&trigger_queue, trig_storage, TRIG_CAPACITY);
    eq_init(&epoch_queue,    epoch_storage,    EPOCH_CAPACITY, TOTAL_FRAMES);
    eq_configure(&epoch_queue, epoch_pool, N_CHANNELS);
    eq_init(&features_queue, features_storage, EPOCH_CAPACITY, TOTAL_FRAMES);
    eq_configure(&features_queue, features_pool, N_CHANNELS);
    eq_init(&output_queue,   output_storage,   EPOCH_CAPACITY, TOTAL_FRAMES);
    eq_configure(&output_queue, output_pool, N_CHANNELS);

    inputs.signal     = &signal_buf;
    inputs.triggers   = &trigger_queue;
    inputs.n_channels = N_CHANNELS;

    config.paradigm          = TBCI_PARADIGM_P300;
    config.nominal_srate     = SRATE;
    config.target_srate      = SRATE;
    config.n_channels        = N_CHANNELS;
    config.window_length_ms  = PRE_MS + POST_MS;
    config.mode              = sliding ? SEG_MODE_SLIDING : SEG_MODE_TRIGGERED;
    config.pre_stimulus_ms   = PRE_MS;
    config.post_stimulus_ms  = POST_MS;
    config.overlap_ms        = sliding ? 400u : 0u;
    config.target_code       = 1;
    config.trial_end_code    = sliding ? 0u  : 10u;
    config.use_preprocessing      = true;
    config.use_feature_extraction = false;  /* ONNX takes raw epoch */
    config.use_decoder            = true;
    config.log_enabled            = true;   /* set true to enable CSV logging */
    config.log_processed          = true;  /* set true to save preprocessed data instead of raw data to CSV */
    config.log_subject[0]         = '\0';
    strncpy(config.log_session, "onnx", sizeof(config.log_session) - 1);


    TBCI_Status s;

    /* preprocessing */
    notch_config.freq_hz = 50.0f;
    notch_config.q_factor   = 30.0f;
    notch_config.n_harmonics = 1;
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

    /* ONNX model */
    strncpy(onnx_config.model_path, "model.onnx", TBCI_ONNX_MAX_PATH_LEN - 1);
    onnx_config.train_trials    = train_trials;
    onnx_config.train_labels    = train_labels;
    onnx_config.train_capacity  = MAX_TRAIN_TRIALS;
    onnx_config.n_folds         = 5;
    onnx_config.output_mode     = TBCI_OUTPUT_SIGMOID;
    onnx_config.sigmoid_threshold = 0.5f;
    onnx_config.scorer          = tbci_score_accuracy;

    group_add_node(&ctx.preprocessing.group, (TBCI_Node *)&notch_node);
    group_add_node(&ctx.preprocessing.group, (TBCI_Node *)&bp_node);
    group_add_node(&ctx.decoder.group,       (TBCI_Node *)&onnx_model);

    s = tbci_context_init(&ctx, &config, &inputs, &processed_buf,
                                       &epoch_queue, &features_queue, &output_queue);
    if (s != TBCI_OK) return s;

    /* init ONNX model after context — needs ctx */
    s = onnx_model_init(&onnx_model, &onnx_config, &ctx);
    if (s != TBCI_OK) {
        fprintf(stderr, "run_onnx: onnx_model_init failed\n");
        return s;
    }

    producer_config.n_channels      = N_CHANNELS;
    producer_config.srate           = SRATE;
    producer_config.freq_hz         = 10.0f;
    producer_config.amplitude       = 1.0f;
    producer_config.noise_amplitude = 0.3f;

    tbci_producer_create_synthetic(producer, &producer_config, &inputs, &ctx);

    tg_config.trigger_interval_ms = 1000u;
    tg_config.trigger_code        = 1u;
    tg_config.trial_end_code      = 0u;
    tg_config.trial_duration_ms   = 0u;

    tg_init(&tg, &tg_config, &tg_state);
    prod.trigger_gen = &tg;

    return TBCI_OK;
}

static void *input_thread(void *arg)
{
    TBCI_Input *inp = (TBCI_Input *)arg;
    int c;
    while ((c = getchar()) != EOF) {
        TBCI_Trigger cmd = { .timestamp_us = now_us() };
        switch (c) {
        case 'i': cmd.code = 193u; break;
        case 't': cmd.code = 194u; break;
        case 's': cmd.code = 192u; break;
        default: continue;
        }
        in_push_trigger(inp, &cmd, &ctx);
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
        .train_trials_bytes  = sizeof(train_trials),
        .train_labels_bytes  = sizeof(train_labels),
        .train_capacity      = MAX_TRAIN_TRIALS,
        .input_size          = TOTAL_FRAMES * N_CHANNELS,
    };

    signal(SIGINT,  handle_signal);
    signal(SIGTERM, handle_signal);

    size_t target_epochs = 10;
    bool sliding = false;

    if (argc > 1) target_epochs = (size_t)atoi(argv[1]);
    if (argc > 2) sliding = (strcmp(argv[2], "sliding") == 0);

    printf("TinyBCI ONNX Runner\n");
    printf("  channels=%d  srate=%.0f Hz  window=%d ms  target=%zu epochs\n\n",
           N_CHANNELS, SRATE, PRE_MS + POST_MS, target_epochs);
    printf("Commands: s=Stop  t=Training  i=Inference\n\n");

    if (init_pipeline(sliding) != TBCI_OK) {
        fprintf(stderr, "Failed to initialise pipeline.\n");
        return 1;
    }
    tbci_print_sizeof(&ctx, &report);

    if (tbci_context_start(&ctx, TBCI_STATE_INFERENCE) != TBCI_OK) {
        fprintf(stderr, "Failed to start pipeline.\n");
        return 1;
    }

    printf("Running — press Ctrl+C to stop\n\n");
    printf("Pipeline running. Reading EEG data...\n");

    size_t   epochs_collected = 0;
    bool     endless          = (target_epochs == 0);
    uint64_t next_tick_us     = now_us();

    pthread_t input_tid;
    pthread_create(&input_tid, NULL, input_thread, &inputs);

    while ((endless || epochs_collected < target_epochs) && running) {
        uint64_t now = now_us();

        if (now >= next_tick_us) {
            TBCI_Status s = producer->tick(producer, &inputs, &ctx);
            if (s != TBCI_OK && s != TBCI_ERR_EMPTY) {
                fprintf(stderr, "Producer error\n");
                break;
            }

            s = tbci_context_tick(&ctx);
            if (s != TBCI_OK) {
                fprintf(stderr, "Pipeline error\n");
                break;
            }

            while (!eq_is_empty(&output_queue)) {
                TBCI_Epoch epoch;
                eq_pop(&output_queue, &epoch);
                print_epoch(&epoch, epochs_collected++);
            }

            uint32_t spacing_us = (uint32_t)(1000000.0f / SRATE);
            next_tick_us += spacing_us;
        }
    }

    onnx_model_close(&onnx_model);
    producer->close(producer);
    tbci_context_stop(&ctx);

    printf("\nDone. Collected %zu epochs.\n", epochs_collected);
    return (epochs_collected == target_epochs || endless) ? EXIT_SUCCESS : EXIT_FAILURE;
}