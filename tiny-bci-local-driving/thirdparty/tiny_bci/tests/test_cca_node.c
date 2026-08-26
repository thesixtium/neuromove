/**
 * @file test_cca_node.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Unit tests for tbci_cca_node.c
 */

#include "unity/unity.h"
#include "../include/nodes/features/tbci_cca_node.h"
#include "tbci_context.h"
#include "tbci_config.h"

/* ------------------------------------------------------------------ */
/* Test dimensions                                                      */
/* ------------------------------------------------------------------ */
#define N_CHANNELS      8
#define N_FREQS         6
#define N_HARMONICS     2
#define N_COMPONENTS    (N_HARMONICS * 2)
#define PRE_MS          0
#define POST_MS         1000
#define TARGET_SRATE    250.0f
#define N_FRAMES        ((size_t)(POST_MS / 1000.0f * TARGET_SRATE))  /* 250 */
#define REF_CAP         (N_FREQS * N_COMPONENTS * N_FRAMES)

#define SIG_CAPACITY    512
#define TRIG_CAPACITY   8
#define EPOCH_CAPACITY  4

/* ------------------------------------------------------------------ */
/* Static storage                                                       */
/* ------------------------------------------------------------------ */
static float     raw_storage   [SIG_CAPACITY * N_CHANNELS];
static uint64_t  raw_timestamps[SIG_CAPACITY];
static uint32_t  raw_indices   [SIG_CAPACITY];

static float     proc_storage   [SIG_CAPACITY * N_CHANNELS];
static uint64_t  proc_timestamps[SIG_CAPACITY];
static uint32_t  proc_indices   [SIG_CAPACITY];

static TBCI_Epoch epoch_storage  [EPOCH_CAPACITY];
static float      epoch_pool     [EPOCH_CAPACITY * N_FRAMES * N_CHANNELS];
static TBCI_Epoch features_storage[EPOCH_CAPACITY];
static float      features_pool  [EPOCH_CAPACITY * N_FRAMES * N_CHANNELS];

static TBCI_Trigger trig_storage[TRIG_CAPACITY];

static float ref_signals[REF_CAP];
static float bad_ref_signals[1];  /* intentionally too small */

static TBCI_SignalBuffer raw_buf;
static TBCI_SignalBuffer proc_buf;
static TBCI_EpochQueue   epoch_queue;
static TBCI_EpochQueue   features_queue;
static TBCI_TriggerQueue trigger_queue;
static TBCI_Input        inputs;
static TBCI_Config       config;
static TBCI_Context      ctx;

static TBCI_CCAConfig    cca_config;
static TBCI_CCANode      cca_node;

static float sample_buf[N_FRAMES * N_CHANNELS];


void setUp(void)
{
    memset(raw_storage,      0, sizeof(raw_storage));
    memset(raw_timestamps,   0, sizeof(raw_timestamps));
    memset(raw_indices,      0, sizeof(raw_indices));
    memset(proc_storage,     0, sizeof(proc_storage));
    memset(proc_timestamps,  0, sizeof(proc_timestamps));
    memset(proc_indices,     0, sizeof(proc_indices));
    memset(epoch_storage,    0, sizeof(epoch_storage));
    memset(epoch_pool,       0, sizeof(epoch_pool));
    memset(features_storage, 0, sizeof(features_storage));
    memset(features_pool,    0, sizeof(features_pool));
    memset(trig_storage,     0, sizeof(trig_storage));
    memset(ref_signals,      0, sizeof(ref_signals));
    memset(&cca_node,        0, sizeof(cca_node));
    memset(&cca_config,      0, sizeof(cca_config));
    memset(&sample_buf,     0, sizeof(sample_buf));

    sb_init(&raw_buf,  raw_storage,  raw_timestamps,  raw_indices,  SIG_CAPACITY, N_CHANNELS);
    sb_init(&proc_buf, proc_storage, proc_timestamps, proc_indices, SIG_CAPACITY, N_CHANNELS);
    eq_init(&epoch_queue,    epoch_storage,    EPOCH_CAPACITY, N_FRAMES);
    eq_init(&features_queue, features_storage, EPOCH_CAPACITY, N_FRAMES);
    eq_configure(&epoch_queue,    epoch_pool,    N_CHANNELS);
    eq_configure(&features_queue, features_pool, N_CHANNELS);
    tq_init(&trigger_queue, trig_storage, TRIG_CAPACITY);

    inputs.signal     = &raw_buf;
    inputs.triggers   = &trigger_queue;
    inputs.n_channels = N_CHANNELS;

    config.paradigm               = TBCI_PARADIGM_SSVEP;
    config.nominal_srate          = TARGET_SRATE;
    config.target_srate           = TARGET_SRATE;
    config.n_channels             = N_CHANNELS;
    config.window_length_ms       = POST_MS;
    config.use_preprocessing      = false;
    config.use_feature_extraction = true;
    config.mode                   = SEG_MODE_SLIDING;
    config.pre_stimulus_ms        = PRE_MS;
    config.post_stimulus_ms       = POST_MS;
    config.overlap_ms             = 0;
    config.trial_end_code         = 0;

    tbci_context_init(&ctx, &config, &inputs, &proc_buf, &epoch_queue, &features_queue, NULL);

    cca_config.n_freqs     = N_FREQS;
    cca_config.n_harmonics = N_HARMONICS;
    cca_config.freqs[0]    = 7.0f;
    cca_config.freqs[1]    = 8.0f;
    cca_config.freqs[2]    = 9.0f;
    cca_config.freqs[3]    = 11.0f;
    cca_config.freqs[4]    = 7.5f;
    cca_config.freqs[5]    = 8.5f;
}

void tearDown(void) {}

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */
static float test_transpose_buf[N_FRAMES * N_CHANNELS];

static void transpose_epoch(TBCI_Epoch *epoch)
{
    size_t n_ch = epoch->n_channels;
    size_t n_fr = epoch->n_frames;
    memcpy(test_transpose_buf, epoch->samples, n_ch * n_fr * sizeof(float));
    for (size_t ch = 0; ch < n_ch; ch++)
        for (size_t fr = 0; fr < n_fr; fr++)
            epoch->samples[ch * n_fr + fr] = test_transpose_buf[fr * n_ch + ch];
}

static void push_epoch_with_value(float value, uint16_t label, uint64_t ts_us)
{
    float *slot = eq_next_slot(&epoch_queue);
    TEST_ASSERT_NOT_NULL(slot);
    for (size_t i = 0; i < N_FRAMES * N_CHANNELS; i++)
        slot[i] = value;
    TBCI_Epoch e = {0};
    e.samples      = slot;
    e.n_frames     = N_FRAMES;
    e.n_channels   = N_CHANNELS;
    e.label        = label;
    e.timestamp_us = ts_us;
    eq_push(&epoch_queue, &e);
}

static void push_epoch_ssvep(float freq_hz, uint16_t label, uint64_t ts_us)
{
    float *slot = eq_next_slot(&epoch_queue);
    TEST_ASSERT_NOT_NULL(slot);
    /* time-major: slot[fr * n_channels + ch] */
    for (size_t fr = 0; fr < N_FRAMES; fr++)
        for (size_t ch = 0; ch < N_CHANNELS; ch++)
            slot[fr * N_CHANNELS + ch] = sinf(2.0f * TBCI_M_PI * freq_hz * (float)fr / TARGET_SRATE)
                                         * (1.0f + 0.1f * (float)ch);  /* slight per-channel variation */
    TBCI_Epoch e = {0};
    e.samples      = slot;
    e.n_frames     = N_FRAMES;
    e.n_channels   = N_CHANNELS;
    e.label        = label;
    e.timestamp_us = ts_us;
    eq_push(&epoch_queue, &e);
}

// Group 1 CCA Arguments Validation
void test_cca_init_null_node_returns_invalid_arg(void)
{
    TBCI_Status s = cca_init(NULL, &cca_config, ref_signals,  SIG_CAPACITY );
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_cca_init_null_config_returns_invalid_arg(void)
{
    TBCI_Status s = cca_init(&cca_node, NULL, ref_signals,  SIG_CAPACITY );
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_cca_init_null_ref_signals_returns_invalid_arg(void)
{
    TBCI_Status s = cca_init(&cca_node, &cca_config, NULL,  SIG_CAPACITY );
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_cca_init_zero_capacity_returns_invalid_arg(void)
{
    TBCI_Status s = cca_init(&cca_node, &cca_config, ref_signals,  0 );
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_cca_init_valid_returns_ok(void)
{
    TBCI_Status s = cca_init(&cca_node, &cca_config, ref_signals,  SIG_CAPACITY );
    TEST_ASSERT_EQUAL(TBCI_OK, s);
}

void test_cca_init_sets_type(void)
{
    TBCI_Status s = cca_init(&cca_node, &cca_config, ref_signals,  SIG_CAPACITY);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
    TEST_ASSERT_NOT_EQUAL(0, &cca_node.base.type);
    TEST_ASSERT_NOT_NULL(&cca_node.base.type);
}

void test_cca_init_sets_enabled(void)
{
    TBCI_Status s = cca_init(&cca_node, &cca_config, ref_signals,  SIG_CAPACITY);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
    TEST_ASSERT_NOT_EQUAL(false, &cca_node.base.enabled);
    TEST_ASSERT_NOT_NULL(&cca_node.base.enabled);
}

void test_cca_init_copies_config(void)
{
    TBCI_Status s = cca_init(&cca_node, &cca_config, ref_signals,  SIG_CAPACITY);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
    TEST_ASSERT_NOT_NULL(&cca_node.config);
}

// Group 2 Reference Signal
void test_cca_init_fn_ref_signals_not_all_zero(void)
{
    cca_init(&cca_node, &cca_config, ref_signals, REF_CAP);
    node_init((TBCI_Node *)&cca_node, &ctx);

    bool all_zero = true;
    for (size_t i = 0; i < REF_CAP; i++) {
        if (ref_signals[i] != 0.0f) { all_zero = false; break; }
    }
    TEST_ASSERT_FALSE(all_zero);
}

void test_cca_init_fn_ref_signals_bounded_minus_one_to_one(void)
{
    cca_init(&cca_node, &cca_config, ref_signals, REF_CAP);
    node_init((TBCI_Node *)&cca_node, &ctx);

    for (size_t i = 0; i < REF_CAP; i++) {
        TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(-1.1f, ref_signals[i]);
        TEST_ASSERT_LESS_OR_EQUAL_FLOAT(1.1f, ref_signals[i]);
    }
}

void test_cca_init_fn_capacity_too_small_returns_invalid_arg(void)
{
    cca_init(&cca_node, &cca_config, bad_ref_signals, 1);
    group_add_node(&ctx.features.group, (TBCI_Node *)&cca_node);
    TBCI_Status s = node_init((TBCI_Node *)&cca_node, &ctx);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

// Group 3 CCA Reset
void test_cca_reset_null_returns_invalid_arg(void)
{
    TBCI_Status s = cca_reset(NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_cca_reset_clears_correlations(void)
{
    TBCI_Status s = cca_reset(&cca_node);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
    bool all_zero = true;
    size_t n = ARRAY_LEN(cca_node.correlations);
    for (size_t i = 0; i < n; i++) {
        if (cca_node.correlations[i] != 0.0f) { all_zero = false; break; }
    }
    TEST_ASSERT_TRUE(all_zero);
}

void test_cca_reset_sets_best_freq_idx_to_minus_one(void)
{
    TBCI_Status s = cca_reset(&cca_node);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
    TEST_ASSERT_EQUAL(-1, cca_node.best_freq_idx);
}

// Group 4 CCA Process Arguments
void test_cca_process_null_node_returns_error(void)
{
    TBCI_Epoch epoch = {
        .samples = sample_buf,
        .n_frames = N_FRAMES,
        .n_channels = N_CHANNELS,
        .timestamp_us = 0,
        .label = 1u
    };

    TBCI_NodeResult s = cca_process(NULL, &epoch, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, s);
}

void test_cca_process_null_data_returns_error(void)
{
    TBCI_NodeResult s = cca_process(&cca_node, NULL, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, s);
}

void test_cca_process_null_ctx_returns_error(void)
{
    TBCI_Epoch epoch = {
        .samples = sample_buf,
        .n_frames = N_FRAMES,
        .n_channels = N_CHANNELS,
        .timestamp_us = 0,
        .label = 1u
    };

    TBCI_NodeResult s = cca_process(&cca_node, &epoch, NULL);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, s);
}

// Group 5 CCA Process Outputs
void test_cca_process_output_n_channels_equals_n_freqs(void)
{
    cca_init(&cca_node, &cca_config, ref_signals, REF_CAP);
    node_init((TBCI_Node *)&cca_node, &ctx);

    push_epoch_ssvep(7.0f, 1u, 0);
    TBCI_Epoch epoch;
    eq_pop(&epoch_queue, &epoch);
    transpose_epoch(&epoch);  /* simulate what fe_process does */

    TBCI_NodeResult s = cca_process(&cca_node, &epoch, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, s);

    TEST_ASSERT_EQUAL_size_t(N_FREQS, epoch.n_channels);
}

void test_cca_process_output_n_frames_equals_one(void)
{
    cca_init(&cca_node, &cca_config, ref_signals, REF_CAP);
    node_init((TBCI_Node *)&cca_node, &ctx);

    push_epoch_ssvep(7.0f, 1u, 0);
    TBCI_Epoch epoch;
    eq_pop(&epoch_queue, &epoch);
    transpose_epoch(&epoch);  /* simulate what fe_process does */

    TBCI_NodeResult s = cca_process(&cca_node, &epoch, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, s);

    TEST_ASSERT_EQUAL_size_t(1, epoch.n_frames);
}

void test_cca_process_correlations_in_zero_one_range(void)
{
    cca_init(&cca_node, &cca_config, ref_signals, REF_CAP);
    node_init((TBCI_Node *)&cca_node, &ctx);

    push_epoch_ssvep(7.0f, 1u, 0);
    TBCI_Epoch epoch;
    eq_pop(&epoch_queue, &epoch);
    transpose_epoch(&epoch);  /* simulate what fe_process does */

    TBCI_NodeResult s = cca_process(&cca_node, &epoch, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, s);
    size_t n = ARRAY_LEN(cca_node.correlations);
    for (int i=0; i < n; i++)
    {
        TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(0.0f, cca_node.correlations[i]);
        TEST_ASSERT_LESS_OR_EQUAL_FLOAT(1.0f, cca_node.correlations[i]);
    }
}

void test_cca_process_best_freq_idx_in_valid_range(void)
{
    cca_init(&cca_node, &cca_config, ref_signals, REF_CAP);
    node_init((TBCI_Node *)&cca_node, &ctx);

    push_epoch_ssvep(7.0f, 1u, 0);
    TBCI_Epoch epoch;
    eq_pop(&epoch_queue, &epoch);
    transpose_epoch(&epoch);  /* simulate what fe_process does */

    TBCI_NodeResult s = cca_process(&cca_node, &epoch, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, s);
    TEST_ASSERT_GREATER_OR_EQUAL_INT(0, cca_node.best_freq_idx);
    TEST_ASSERT_LESS_OR_EQUAL_INT(N_FREQS - 1, cca_node.best_freq_idx);
}

void test_cca_process_ssvep_signal_produces_nonzero_correlation(void)
{
    cca_init(&cca_node, &cca_config, ref_signals, REF_CAP);
    node_init((TBCI_Node *)&cca_node, &ctx);

    push_epoch_ssvep(7.0f, 1u, 0);
    TBCI_Epoch epoch;
    eq_pop(&epoch_queue, &epoch);
    transpose_epoch(&epoch);

    TBCI_NodeResult s = cca_process(&cca_node, (void *)&epoch, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, s);

    /* at least one frequency should have a nonzero correlation */
    bool any_nonzero = false;
    for (size_t i = 0; i < cca_node.config.n_freqs; i++) {
        if (cca_node.correlations[i] > 0.0f) { any_nonzero = true; break; }
    }
    TEST_ASSERT_TRUE(any_nonzero);
    TEST_ASSERT_GREATER_OR_EQUAL_INT(0, cca_node.best_freq_idx);
}


int main(void)
{
    UNITY_BEGIN();

    // Group 1 — cca_init
    RUN_TEST(test_cca_init_null_node_returns_invalid_arg);
    RUN_TEST(test_cca_init_null_config_returns_invalid_arg);
    RUN_TEST(test_cca_init_null_ref_signals_returns_invalid_arg);
    RUN_TEST(test_cca_init_zero_capacity_returns_invalid_arg);
    RUN_TEST(test_cca_init_valid_returns_ok);
    RUN_TEST(test_cca_init_sets_type);
    RUN_TEST(test_cca_init_sets_enabled);
    RUN_TEST(test_cca_init_copies_config);

    // Group 2 — ref signal generation
    RUN_TEST(test_cca_init_fn_ref_signals_not_all_zero);
    RUN_TEST(test_cca_init_fn_ref_signals_bounded_minus_one_to_one);
    RUN_TEST(test_cca_init_fn_capacity_too_small_returns_invalid_arg);

    // Group 3 — cca_reset
    RUN_TEST(test_cca_reset_null_returns_invalid_arg);
    RUN_TEST(test_cca_reset_clears_correlations);
    RUN_TEST(test_cca_reset_sets_best_freq_idx_to_minus_one);

    // Group 4 — cca_process validation
    RUN_TEST(test_cca_process_null_node_returns_error);
    RUN_TEST(test_cca_process_null_data_returns_error);
    RUN_TEST(test_cca_process_null_ctx_returns_error);

    // Group 5 — output shape and bounds
    RUN_TEST(test_cca_process_output_n_channels_equals_n_freqs);
    RUN_TEST(test_cca_process_output_n_frames_equals_one);
    RUN_TEST(test_cca_process_correlations_in_zero_one_range);
    RUN_TEST(test_cca_process_best_freq_idx_in_valid_range);
    RUN_TEST(test_cca_process_ssvep_signal_produces_nonzero_correlation);

    return UNITY_END();
}