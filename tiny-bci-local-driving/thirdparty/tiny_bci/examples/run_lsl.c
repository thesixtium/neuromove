/**
 * @file run_lsl.c
 *
 * @author Ashley Exall
 *
 * @brief Minimal TinyBCI runner using an LSL producer.
 *
 * Demonstrates the full pipeline:
 *   LSLProducer → TBCI_Input → SegmentationNode → TBCI_EpochQueue
 *
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

#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include "../producer/lsl_producer.h"
#include "tbci_context.h"
#include "tbci_config.h"
#include "../include/ioutils/tbci_input.h"
#include "../include/containers/tbci_signal_buffer.h"
#include "../include/containers/tbci_trigger_queue.h"
#include "../include/containers/tbci_epoch_queue.h"
#include "tbci_producer_factory.h"
#include "../include/mathutils/tbci_sizeof.h"
#include "../include/ioutils/tbci_lsl_writer.h"
#include "tbci_platform.h"
#include "../include/nodes/decoder/tbci_cca_model.h"
#include "../include/nodes/features/tbci_cca_node.h"
#include "nodes/preprocessing/tbci_bandpass_node.h"
#include "nodes/preprocessing/tbci_notch_node.h"

/* --------------------------------------------------------------------------
 * Capacity constants
 * -------------------------------------------------------------------------- */

#define MAX_CHANNELS       64
#define SIG_BUF_CAPACITY  1024
#define TRIG_QUEUE_CAP     32
#define EPOCH_QUEUE_CAP    16
#define EPOCH_N_FRAMES_MAX 512

#define EEG_STREAM    "eeg"
#define MARKER_STREAM "Markers"

#define POST_STIMULUS_MS  800u
#define PRE_STIMULUS_MS   200u

/* --------------------------------------------------------------------------
 * Static storage — all caller-provided, no malloc
 * -------------------------------------------------------------------------- */

static float    sig_storage   [SIG_BUF_CAPACITY * MAX_CHANNELS];
static uint64_t sig_timestamps[SIG_BUF_CAPACITY];
static uint32_t sig_indices   [SIG_BUF_CAPACITY];

static float    proc_storage   [SIG_BUF_CAPACITY * MAX_CHANNELS];
static uint64_t proc_timestamps[SIG_BUF_CAPACITY];
static uint32_t proc_indices   [SIG_BUF_CAPACITY];

static TBCI_Trigger trig_storage    [TRIG_QUEUE_CAP];
static TBCI_Epoch   epoch_storage   [EPOCH_QUEUE_CAP];
static float        epoch_pool      [EPOCH_QUEUE_CAP * EPOCH_N_FRAMES_MAX * MAX_CHANNELS];
static TBCI_Epoch   features_storage[EPOCH_QUEUE_CAP];
static float        features_pool   [EPOCH_QUEUE_CAP * EPOCH_N_FRAMES_MAX * MAX_CHANNELS];
static TBCI_Epoch   output_storage  [EPOCH_QUEUE_CAP];
static float        output_pool     [EPOCH_QUEUE_CAP * EPOCH_N_FRAMES_MAX * MAX_CHANNELS];

static float lsl_temp_buffer[MAX_CHANNELS];

static TBCI_SignalBuffer sig_buf;
static TBCI_SignalBuffer proc_buf;
static TBCI_TriggerQueue trig_queue;
static TBCI_EpochQueue   epoch_queue;
static TBCI_EpochQueue   features_queue;
static TBCI_EpochQueue   output_queue;
static TBCI_Input        inputs = {&sig_buf, &trig_queue, MAX_CHANNELS};
static TBCI_Config       config;
static TBCI_Context      ctx;

static LSLProducerConfig lsl_cfg;
static LSLProducer prod;
TBCI_Producer *producer = (TBCI_Producer *)&prod;
static TBCI_LSLWriter lsl_writer;

// CCA constants
#define N_FREQS         6
#define N_HARMONICS     2
#define N_COMPONENTS    (N_HARMONICS * 2)
#define REF_CAP         (N_FREQS * N_COMPONENTS * EPOCH_N_FRAMES_MAX)
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
static float srate = 250.0f;

static void handle_signal(int sig)
{
    (void)sig;
    running = false;
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
    /* 1. Buffers */
    sb_init(&sig_buf,  sig_storage,  sig_timestamps, sig_indices, SIG_BUF_CAPACITY, MAX_CHANNELS);
    sb_init(&proc_buf, proc_storage, proc_timestamps, proc_indices,SIG_BUF_CAPACITY, MAX_CHANNELS);
    tq_init(&trig_queue, trig_storage, TRIG_QUEUE_CAP);

    inputs.signal     = &sig_buf;
    inputs.triggers   = &trig_queue;
    inputs.n_channels = MAX_CHANNELS;

    /* 2. LSL producer */
    lsl_cfg.data_stream   = EEG_STREAM;
    lsl_cfg.marker_stream = MARKER_STREAM;
    lsl_cfg.mode               = LSL_MODE_EEG_AND_MARKERS;
    lsl_cfg.temp_buf           = lsl_temp_buffer;
    lsl_cfg.max_channels       = MAX_CHANNELS;
    lsl_cfg.resolve_mode = LSL_RESOLVE_BY_TYPE;

    printf("Waiting for LSL streams '%s' and '%s'...\n",
           EEG_STREAM, MARKER_STREAM);

    TBCI_Status s = tbci_producer_create_lsl(producer, &lsl_cfg, &inputs, NULL);
    if (s != TBCI_OK) return s;

    /* 3. Read stream metadata */
    int   n_channels = lp_get_n_channels(&prod);
    srate      = lp_get_srate(&prod);


    size_t epoch_n_frames = (size_t)(srate * (PRE_STIMULUS_MS + POST_STIMULUS_MS) / 1000.0f);

    inputs.n_channels = (size_t)n_channels;

    /* 4. Epoch + features queues — sized from discovered metadata */
    eq_init(&epoch_queue, epoch_storage, EPOCH_QUEUE_CAP, epoch_n_frames);
    eq_configure(&epoch_queue, epoch_pool, (size_t)n_channels);

    eq_init(&features_queue, features_storage, EPOCH_QUEUE_CAP, epoch_n_frames);
    eq_configure(&features_queue, features_pool, (size_t)n_channels);

    eq_init(&output_queue, output_storage, EPOCH_QUEUE_CAP, epoch_n_frames);
    eq_configure(&output_queue, output_pool, (size_t)n_channels);

    /* 5. Pipeline context */
    config.paradigm               = sliding ? TBCI_PARADIGM_MI : TBCI_PARADIGM_P300;;
    config.nominal_srate          = srate;
    config.target_srate           = srate;
    config.n_channels             = (size_t)n_channels;
    config.window_length_ms       = PRE_STIMULUS_MS + POST_STIMULUS_MS;
    config.mode                   = sliding ? SEG_MODE_SLIDING : SEG_MODE_TRIGGERED;
    config.pre_stimulus_ms        = PRE_STIMULUS_MS;
    config.post_stimulus_ms       = POST_STIMULUS_MS;
    config.overlap_ms        = sliding ? 400u : 0u;
    config.target_code       = 1;
    config.trial_end_code    = sliding ? 10u  : 0u;
    config.use_preprocessing      = true;
    config.use_feature_extraction = true;
    config.use_decoder            = true;
    config.log_enabled            = true;  /* set true to enable CSV logging */
    config.log_processed          = true;  /* set true to save preprocessed data instead of raw data to CSV */
    config.log_subject[0]         = '\0';
    strncpy(config.log_session, "lsl", sizeof(config.log_session) - 1);
    config.log_session[sizeof(config.log_session) - 1] = '\0';

    /* register notch & bandpass node in preprocessing group */
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

    s = tbci_context_init(&ctx, &config, &inputs, &proc_buf, &epoch_queue, &features_queue, &output_queue);
    if (s != TBCI_OK)
        return TBCI_ERR_INVALID_STATE;

    if (!lsl_writer_init(&lsl_writer, "TinyBCI_Out", "Predictions", (int)config.n_channels, 0.0f)) {
        fprintf(stderr, "Warning: LSL writer init failed — results will not be streamed\n");}

    printf("Connected: %d channels @ %.1f Hz\n", n_channels, srate);
    printf("Epoch window: %u ms = %zu frames\n", PRE_STIMULUS_MS + POST_STIMULUS_MS, epoch_n_frames);
    return TBCI_OK;

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
        .input_size          = EPOCH_N_FRAMES_MAX * MAX_CHANNELS,
    };

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    size_t target_epochs = 10;
    bool sliding = false;

    if (argc > 1) target_epochs = (size_t)atoi(argv[1]);
    if (argc > 2) sliding = (strcmp(argv[2], "sliding") == 0);

    if (init_pipeline(sliding) != TBCI_OK) {
        fprintf(stderr, "Failed to initialise pipeline.\n");
        return TBCI_ERR_INVALID_STATE;
    }

    tbci_print_sizeof(&ctx, &report);

    if (tbci_context_start(&ctx, TBCI_STATE_INFERENCE) != TBCI_OK) {
        fprintf(stderr, "Failed to start TinyBCI pipeline.\n");
        return 1;
    }

    printf("Running — press Ctrl+C to stop\n\n");
    printf("Pipeline running. Waiting for EEG data...\n");

    size_t epochs_collected = 0;
    bool endless = (target_epochs == 0);
    unsigned int sleep_us = (unsigned int)(1000000.0f / config.nominal_srate);

    while ((endless || epochs_collected < target_epochs) && running) {
        if (producer->tick(producer, &inputs, &ctx) != TBCI_OK) {
            fprintf(stderr, "Producer tick error.\n");
            break;
        }

        /* process pipeline */
        TBCI_Status status = tbci_context_tick(&ctx);
        if (status != TBCI_OK) {
            fprintf(stderr, "Pipeline error\n");
            break;
        }

        while (!eq_is_empty(&output_queue)) {
            TBCI_Epoch epoch;
            eq_pop(&output_queue, &epoch);
            print_epoch(&epoch, epochs_collected++);
            lsl_writer_push_epoch(&lsl_writer, &epoch);
        }

        tbci_sleep_us(sleep_us);
    }

    producer->close(producer);
    tbci_context_stop(&ctx);
    lsl_writer_close(&lsl_writer);

    printf("\nDone. Collected %zu / %zu epochs.\n", epochs_collected, target_epochs);
    return (epochs_collected == target_epochs) ? EXIT_SUCCESS : EXIT_FAILURE;
}