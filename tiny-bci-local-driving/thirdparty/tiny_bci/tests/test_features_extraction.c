/**
 * @file test_features_extraction.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Unit tests for tbci_features_extraction.c (fe_init / fe_process / fe_reset)
 *        using the Unity test framework.
 *
 * @note Assumes TBCI_Context exposes the raw signal buffer via
 *       ctx.inputs->signal and the processed buffer via ctx.processed_buf
 *       (a TBCI_SignalBuffer*, set by tbci_context_init's 4th argument).
 *       Adjust field names below if your TBCI_Context differs.
 */

#include "unity/unity.h"
#include "../include/nodes/features/tbci_features_extraction.h"
#include "tbci_context.h"
#include "tbci_config.h"

#define SIG_CAPACITY  16
#define SIG_CHANNELS  4
#define TRIG_CAPACITY 4
#define EPOCH_CAPACITY 4
#define EPOCH_N_FRAMES  16
#define TARGET_SRATE  256.0f

static float    raw_storage[SIG_CAPACITY * SIG_CHANNELS];
static uint64_t raw_timestamps[SIG_CAPACITY];
static uint32_t raw_indices[SIG_CAPACITY];

static float    proc_storage[SIG_CAPACITY * SIG_CHANNELS];
static uint64_t proc_timestamps[SIG_CAPACITY];
static uint32_t proc_indices[SIG_CAPACITY];

static TBCI_Epoch epoch_storage[EPOCH_CAPACITY];
static float      epoch_pool[EPOCH_CAPACITY * EPOCH_N_FRAMES * SIG_CHANNELS];

static TBCI_Epoch features_storage[EPOCH_CAPACITY];
static float      features_pool[EPOCH_CAPACITY * EPOCH_N_FRAMES * SIG_CHANNELS];

static TBCI_Trigger trig_storage[TRIG_CAPACITY];

static TBCI_SignalBuffer raw_buf;
static TBCI_SignalBuffer proc_buf;
static TBCI_EpochQueue   epoch_queue;
static TBCI_EpochQueue   features_queue;
static TBCI_TriggerQueue trigger_queue;
static TBCI_Input        inputs;
static TBCI_Config       config;
static TBCI_Context      ctx;

static TBCI_FeatureExtraction fe;

/* mock inner node — doubles every channel */
static TBCI_NodeResult double_process(TBCI_Node *self, void *data, struct TBCI_Context *c)
{
    (void)self; (void)c;
    TBCI_Epoch *epoch = (TBCI_Epoch *)data;
    for (size_t i = 0; i < epoch->n_frames * epoch->n_channels; i++)
        epoch->samples[i] *= 2.0f;
    return TBCI_NODE_OK;
}

static int init_calls;
static int reset_calls;

static TBCI_Status double_init(TBCI_Node *self, struct TBCI_Context *c)
{
    (void)self; (void)c;
    init_calls++;
    return TBCI_OK;
}

static TBCI_Status double_reset(TBCI_Node *self)
{
    (void)self;
    reset_calls++;
    return TBCI_OK;
}

static TBCI_Node double_node;

void setUp(void)
{
    memset(raw_storage,     0, sizeof(raw_storage));
    memset(raw_timestamps,  0, sizeof(raw_timestamps));
    memset(raw_indices,     0, sizeof(raw_indices));
    memset(proc_storage,    0, sizeof(proc_storage));
    memset(proc_timestamps, 0, sizeof(proc_timestamps));
    memset(proc_indices,    0, sizeof(proc_indices));
    memset(epoch_storage,   0, sizeof(epoch_storage));
    memset(epoch_pool,   0, sizeof(epoch_pool));
    memset(features_storage,   0, sizeof(features_storage));
    memset(features_pool,   0, sizeof(features_pool));
    memset(trig_storage,    0, sizeof(trig_storage));
    memset(&double_node,    0, sizeof(double_node));
    memset(&fe,             0, sizeof(fe));

    init_calls  = 0;
    reset_calls = 0;

    sb_init(&raw_buf,  raw_storage,  raw_timestamps,  raw_indices,  SIG_CAPACITY, SIG_CHANNELS);
    sb_init(&proc_buf, proc_storage, proc_timestamps, proc_indices, SIG_CAPACITY, SIG_CHANNELS);
    eq_init(&epoch_queue, epoch_storage, EPOCH_CAPACITY, EPOCH_N_FRAMES);
    eq_init(&features_queue, features_storage, EPOCH_CAPACITY, EPOCH_N_FRAMES);
    eq_configure(&epoch_queue, epoch_pool, SIG_CHANNELS);
    eq_configure(&features_queue, features_pool, SIG_CHANNELS);
    tq_init(&trigger_queue, trig_storage, TRIG_CAPACITY);


    inputs.signal     = &raw_buf;
    inputs.triggers   = &trigger_queue;
    inputs.n_channels = SIG_CHANNELS;

    config.paradigm               = TBCI_PARADIGM_P300;
    config.nominal_srate          = TARGET_SRATE;
    config.target_srate           = TARGET_SRATE;
    config.n_channels             = SIG_CHANNELS;
    config.window_length_ms       = 1000;
    config.use_preprocessing      = true;
    config.use_feature_extraction = false;
    config.mode                   = SEG_MODE_TRIGGERED;
    config.pre_stimulus_ms        = 0;
    config.post_stimulus_ms       = 100;
    config.overlap_ms             = 0;
    config.trial_end_code         = 0;

    tbci_context_init(&ctx, &config, &inputs, &proc_buf, &epoch_queue, &features_queue, NULL);

    double_node.enabled    = true;
    double_node.init_fn = double_init;
    double_node.process_fn = double_process;
    double_node.reset_fn   = double_reset;
}

void tearDown(void) {}

/* helper — push a minimal epoch into epoch_queue */
static void push_test_epoch(float value, uint16_t label, uint64_t ts_us)
{
    float *slot = eq_next_slot(&epoch_queue);
    TEST_ASSERT_NOT_NULL(slot);

    /* fill pool slot directly */
    for (size_t i = 0; i < EPOCH_N_FRAMES * SIG_CHANNELS; i++)
        slot[i] = value;
    TBCI_Epoch epoch = {0};
    epoch.label        = label;
    epoch.timestamp_us = ts_us;
    epoch.n_frames     = EPOCH_N_FRAMES;
    epoch.n_channels   = SIG_CHANNELS;
    epoch.samples = slot;

    eq_push(ctx.epoch_queue, &epoch);
}

static void push_test_epoch_sequential(uint16_t label, uint64_t ts_us)
{
    float *slot = eq_next_slot(&epoch_queue);
    TEST_ASSERT_NOT_NULL(slot);

    /* fill time-major: sample value = frame * n_channels + channel (1-based) */
    for (size_t fr = 0; fr < EPOCH_N_FRAMES; fr++)
        for (size_t ch = 0; ch < SIG_CHANNELS; ch++)
            slot[fr * SIG_CHANNELS + ch] = (float)(fr * SIG_CHANNELS + ch + 1);

    TBCI_Epoch epoch = {0};
    epoch.label        = label;
    epoch.timestamp_us = ts_us;
    epoch.n_frames     = EPOCH_N_FRAMES;
    epoch.n_channels   = SIG_CHANNELS;
    epoch.samples      = slot;

    eq_push(ctx.epoch_queue, &epoch);
}
/* ============================================================
 * GROUP 1 — fe_init
 * ============================================================ */

void test_fe_init_null_node_returns_invalid_arg(void)
{
    TBCI_Status s = fe_init(NULL, true, &ctx);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_fe_init_null_ctx_returns_invalid_arg(void)
{
    TBCI_Status s = fe_init(&fe, true, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_fe_init_sets_type(void)
{
    fe_init(&fe, true, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_TYPE_FEATURE_EXTRACTION, fe.group.base.type);
}

void test_fe_init_enabled_flag_set(void)
{
    fe_init(&fe, true, &ctx);
    TEST_ASSERT_TRUE(fe.extraction_enabled);

    fe_init(&fe, false, &ctx);
    TEST_ASSERT_FALSE(fe.extraction_enabled);
}

void test_fe_init_n_nodes_zero_by_default(void)
{
    fe_init(&fe, true, &ctx);
    TEST_ASSERT_EQUAL_size_t(0, fe.group.n_nodes);
}

void test_fe_init_preserves_caller_registered_nodes(void)
{
    group_add_node(&fe.group, &double_node);

    fe_init(&fe, true, &ctx);

    TEST_ASSERT_EQUAL_size_t(1, fe.group.n_nodes);
    TEST_ASSERT_EQUAL_PTR(&double_node, fe.group.nodes[0]);
}

void test_fe_init_calls_init_on_registered_nodes(void)
{
    group_add_node(&fe.group, &double_node);

    TBCI_Status s = fe_init(&fe, true, &ctx);

    TEST_ASSERT_EQUAL(TBCI_OK, s);
    TEST_ASSERT_EQUAL(1, init_calls);
}

/* ============================================================
 * GROUP 2 — fe_process: argument validation & empty buffer
 * ============================================================ */

void test_fe_process_null_node_returns_error(void)
{
    TBCI_NodeResult r = fe_process(NULL, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, r);
}

void test_fe_process_null_ctx_returns_error(void)
{
    fe_init(&fe, true, &ctx);
    TBCI_NodeResult r = fe_process(&fe, NULL);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, r);
}


/* ============================================================
 * GROUP 3 — fe_process: pass-through (no inner nodes)
 * ============================================================ */

void test_fe_process_enabled_no_nodes_moves_epoch_to_feature_queue(void)
{
    fe_init(&fe, true, &ctx);
    push_test_epoch(1.5f, 1u, 1000u);

    TBCI_NodeResult r = fe_process(&fe, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, r);

    /* epoch_queue should now be empty — epoch was consumed */
    TEST_ASSERT_TRUE(eq_is_empty(ctx.epoch_queue));

    /* feature_queue should have one epoch */
    TEST_ASSERT_FALSE(eq_is_empty(ctx.features_queue));

    TBCI_Epoch out;
    eq_pop(ctx.features_queue, &out);
    TEST_ASSERT_EQUAL_UINT16(1u, out.label);
    TEST_ASSERT_EQUAL_UINT64(1000u, out.timestamp_us);
    TEST_ASSERT_EQUAL_size_t(EPOCH_N_FRAMES, out.n_frames);
    TEST_ASSERT_EQUAL_size_t(SIG_CHANNELS,   out.n_channels);
}

void test_fe_process_disabled_still_moves_epoch_to_feature_queue(void)
{
    fe_init(&fe, false, &ctx);
    push_test_epoch(2.0f, 2u, 2000u);

    TBCI_NodeResult r = fe_process(&fe, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, r);

    /* disabled still passes through — feature_queue gets the epoch */
    TEST_ASSERT_FALSE(eq_is_empty(ctx.features_queue));

    TBCI_Epoch out;
    eq_pop(ctx.features_queue, &out);
    TEST_ASSERT_EQUAL_UINT16(2u, out.label);
    TEST_ASSERT_EQUAL_UINT64(2000u, out.timestamp_us);
}

void test_fe_process_empty_epoch_queue_returns_pending(void)
{
    fe_init(&fe, true, &ctx);
    TBCI_NodeResult r = fe_process(&fe, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_PENDING, r);
}

void test_fe_process_multiple_epochs_moves_one_per_tick(void)
{
    fe_init(&fe, true, &ctx);
    push_test_epoch(1.0f, 1u, 1000u);
    push_test_epoch(2.0f, 2u, 2000u);

    /* first tick — moves first epoch */
    fe_process(&fe, &ctx);
    TEST_ASSERT_EQUAL_size_t(1, eq_size(ctx.features_queue));

    /* second tick — moves second epoch */
    fe_process(&fe, &ctx);
    TEST_ASSERT_EQUAL_size_t(2, eq_size(ctx.features_queue));

    /* epoch_queue now empty */
    TEST_ASSERT_TRUE(eq_is_empty(ctx.epoch_queue));
}

void test_fe_process_preserves_epoch_samples_in_passthrough(void)
{
    fe_init(&fe, true, &ctx);
    push_test_epoch(3.0f, 1u, 3000u);

    fe_process(&fe, &ctx);

    TBCI_Epoch out;
    eq_pop(ctx.features_queue, &out);

    /* all samples should equal 3.0 — unchanged */
    for (size_t i = 0; i < EPOCH_N_FRAMES * SIG_CHANNELS; i++)
        TEST_ASSERT_EQUAL_FLOAT(3.0f, out.samples[i]);
}

/* ============================================================
 * GROUP 4 — fe_process: with one inner node
 * ============================================================ */

void test_fe_process_enabled_with_node_applies_transform(void)
{
    group_add_node(&fe.group, &double_node);
    fe_init(&fe, true, &ctx);
    push_test_epoch(3.0f, 1u, 3000u);

    TBCI_NodeResult r = fe_process(&fe, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, r);

    TBCI_Epoch out;
    eq_pop(ctx.features_queue, &out);

    /* double_node doubles all samples: 3.0 * 2 = 6.0 */
    for (size_t i = 0; i < EPOCH_N_FRAMES * SIG_CHANNELS; i++)
        TEST_ASSERT_EQUAL_FLOAT(6.0f, out.samples[i]);
}

void test_fe_process_disabled_group_skips_node_transform(void)
{
    group_add_node(&fe.group, &double_node);
    fe_init(&fe, false, &ctx);
    push_test_epoch(3.0f, 1u, 3000u);

    TBCI_NodeResult r = fe_process(&fe, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, r);

    TBCI_Epoch out;
    eq_pop(ctx.features_queue, &out);

    /* extraction disabled — samples unchanged */
    for (size_t i = 0; i < EPOCH_N_FRAMES * SIG_CHANNELS; i++)
        TEST_ASSERT_EQUAL_FLOAT(3.0f, out.samples[i]);
}

/* ============================================================
 * GROUP 5 — fe_reset
 * ============================================================ */

void test_fe_reset_null_returns_invalid_arg(void)
{
    TBCI_Status s = fe_reset(NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_fe_reset_calls_inner_node_reset(void)
{
    group_add_node(&fe.group, &double_node);
    fe_init(&fe, true, &ctx);

    TBCI_Status s = fe_reset(&fe);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
    TEST_ASSERT_EQUAL(1, reset_calls);
}

/* ============================================================
 * GROUP 6 — fe_transpose_epoch
 * ============================================================ */

void test_fe_transpose_basic_correctness(void)
{
    fe_init(&fe, true, &ctx);
    push_test_epoch_sequential(1u, 1000u);

    fe_process(&fe, &ctx);

    TBCI_Epoch out;
    eq_pop(ctx.features_queue, &out);

    /* verify channel-major layout: out.samples[ch * n_frames + fr] */
    for (size_t ch = 0; ch < SIG_CHANNELS; ch++)
        for (size_t fr = 0; fr < EPOCH_N_FRAMES; fr++)
            TEST_ASSERT_EQUAL_FLOAT(
                (float)(fr * SIG_CHANNELS + ch + 1),
                out.samples[ch * EPOCH_N_FRAMES + fr]
            );
}

void test_fe_transpose_single_channel(void)
{
    /* single channel — layout unchanged by transpose */
    float *slot = eq_next_slot(&epoch_queue);
    TEST_ASSERT_NOT_NULL(slot);

    for (size_t fr = 0; fr < EPOCH_N_FRAMES; fr++)
        slot[fr] = (float)(fr + 1);

    TBCI_Epoch epoch = {0};
    epoch.label        = 1u;
    epoch.timestamp_us = 1000u;
    epoch.n_frames     = EPOCH_N_FRAMES;
    epoch.n_channels   = 1;
    epoch.samples      = slot;
    eq_push(ctx.epoch_queue, &epoch);

    fe_init(&fe, true, &ctx);
    fe_process(&fe, &ctx);

    TBCI_Epoch out;
    eq_pop(ctx.features_queue, &out);

    for (size_t fr = 0; fr < EPOCH_N_FRAMES; fr++)
        TEST_ASSERT_EQUAL_FLOAT((float)(fr + 1), out.samples[fr]);
}

void test_fe_transpose_single_frame(void)
{
    /* single frame — layout unchanged by transpose */
    float *slot = eq_next_slot(&epoch_queue);
    TEST_ASSERT_NOT_NULL(slot);

    for (size_t ch = 0; ch < SIG_CHANNELS; ch++)
        slot[ch] = (float)(ch + 1);

    TBCI_Epoch epoch = {0};
    epoch.label        = 1u;
    epoch.timestamp_us = 1000u;
    epoch.n_frames     = 1;
    epoch.n_channels   = SIG_CHANNELS;
    epoch.samples      = slot;
    eq_push(ctx.epoch_queue, &epoch);

    fe_init(&fe, true, &ctx);
    fe_process(&fe, &ctx);

    TBCI_Epoch out;
    eq_pop(ctx.features_queue, &out);

    for (size_t ch = 0; ch < SIG_CHANNELS; ch++)
        TEST_ASSERT_EQUAL_FLOAT((float)(ch + 1), out.samples[ch]);
}

void test_fe_transpose_preserves_label_and_timestamp(void)
{
    fe_init(&fe, true, &ctx);
    push_test_epoch_sequential(42u, 99000u);

    fe_process(&fe, &ctx);

    TBCI_Epoch out;
    eq_pop(ctx.features_queue, &out);

    TEST_ASSERT_EQUAL_UINT16(42u, out.label);
    TEST_ASSERT_EQUAL_UINT64(99000u, out.timestamp_us);
}

int main(void)
{
    UNITY_BEGIN();

    // Group 1 — fe_init
    RUN_TEST(test_fe_init_null_node_returns_invalid_arg);
    RUN_TEST(test_fe_init_null_ctx_returns_invalid_arg);
    RUN_TEST(test_fe_init_sets_type);
    RUN_TEST(test_fe_init_enabled_flag_set);
    RUN_TEST(test_fe_init_n_nodes_zero_by_default);
    RUN_TEST(test_fe_init_preserves_caller_registered_nodes);
    RUN_TEST(test_fe_init_calls_init_on_registered_nodes);

    // Group 2 — fe_process validation
    RUN_TEST(test_fe_process_null_node_returns_error);
    RUN_TEST(test_fe_process_null_ctx_returns_error);

    // Group 3 — pass-through
    RUN_TEST(test_fe_process_enabled_no_nodes_moves_epoch_to_feature_queue);
    RUN_TEST(test_fe_process_disabled_still_moves_epoch_to_feature_queue);
    RUN_TEST(test_fe_process_empty_epoch_queue_returns_pending);
    RUN_TEST(test_fe_process_multiple_epochs_moves_one_per_tick);
    RUN_TEST(test_fe_process_preserves_epoch_samples_in_passthrough);

    // Group 4 — with inner node
    RUN_TEST(test_fe_process_enabled_with_node_applies_transform);
    RUN_TEST(test_fe_process_disabled_group_skips_node_transform);

    // Group 5 — fe_reset
    RUN_TEST(test_fe_reset_null_returns_invalid_arg);
    RUN_TEST(test_fe_reset_calls_inner_node_reset);

    // Group 6 — transpose
    RUN_TEST(test_fe_transpose_basic_correctness);
    RUN_TEST(test_fe_transpose_single_channel);
    RUN_TEST(test_fe_transpose_single_frame);
    RUN_TEST(test_fe_transpose_preserves_label_and_timestamp);

    return UNITY_END();
}