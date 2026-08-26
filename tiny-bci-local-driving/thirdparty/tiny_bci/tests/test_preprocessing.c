/**
 * @file test_preprocessing.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Unit tests for tbci_preprocessing.c (pp_init / pp_process / pp_reset)
 *        using the Unity test framework.
 *
 * @note Assumes TBCI_Context exposes the raw signal buffer via
 *       ctx.inputs->signal and the processed buffer via ctx.processed_buf
 *       (a TBCI_SignalBuffer*, set by tbci_context_init's 4th argument).
 *       Adjust field names below if your TBCI_Context differs.
 */

#include "unity/unity.h"
#include "../include/nodes/preprocessing/tbci_preprocessing.h"
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

static TBCI_Preprocessing pp;

/* mock inner node — doubles every channel */
static TBCI_NodeResult double_process(TBCI_Node *self, void *data, struct TBCI_Context *c)
{
    (void)self; (void)c;
    float *samples = (float *)data;
    for (size_t i = 0; i < SIG_CHANNELS; i++)
        samples[i] *= 2.0f;
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
    memset(&pp,             0, sizeof(pp));

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

static void push_raw_frame(float v, uint64_t ts_us, uint32_t idx)
{
    float samples[SIG_CHANNELS];
    for (size_t i = 0; i < SIG_CHANNELS; i++) samples[i] = v;
    sb_put(&raw_buf, samples, ts_us, idx);
}
/* ============================================================
 * GROUP 1 — pp_init
 * ============================================================ */

void test_pp_init_null_node_returns_invalid_arg(void)
{
    TBCI_Status s = pp_init(NULL, true, &ctx);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_pp_init_null_ctx_returns_invalid_arg(void)
{
    TBCI_Status s = pp_init(&pp, true, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_pp_init_sets_type(void)
{
    pp_init(&pp, true, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_TYPE_PREPROCESSING, pp.group.base.type);
}

void test_pp_init_enabled_flag_set(void)
{
    pp_init(&pp, true, &ctx);
    TEST_ASSERT_TRUE(pp.filtering_enabled);

    pp_init(&pp, false, &ctx);
    TEST_ASSERT_FALSE(pp.filtering_enabled);
}

void test_pp_init_n_nodes_zero_by_default(void)
{
    pp_init(&pp, true, &ctx);
    TEST_ASSERT_EQUAL_size_t(0, pp.group.n_nodes);
}

void test_pp_init_preserves_caller_registered_nodes(void)
{
    group_add_node(&pp.group, &double_node);

    pp_init(&pp, true, &ctx);

    TEST_ASSERT_EQUAL_size_t(1, pp.group.n_nodes);
    TEST_ASSERT_EQUAL_PTR(&double_node, pp.group.nodes[0]);
}

void test_pp_init_calls_init_on_registered_nodes(void)
{
    group_add_node(&pp.group, &double_node);

    TBCI_Status s = pp_init(&pp, true, &ctx);

    TEST_ASSERT_EQUAL(TBCI_OK, s);
    TEST_ASSERT_EQUAL(1, init_calls);
}

/* ============================================================
 * GROUP 2 — pp_process: argument validation & empty buffer
 * ============================================================ */

void test_pp_process_null_node_returns_error(void)
{
    TBCI_NodeResult r = pp_process(NULL, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, r);
}

void test_pp_process_null_ctx_returns_error(void)
{
    pp_init(&pp, true, &ctx);
    TBCI_NodeResult r = pp_process(&pp, NULL);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, r);
}

void test_pp_process_empty_signal_buffer_returns_pending(void)
{
    pp_init(&pp, true, &ctx);
    TBCI_NodeResult r = pp_process(&pp, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_PENDING, r);
}

/* ============================================================
 * GROUP 3 — pp_process: pass-through (no inner nodes)
 * ============================================================ */

void test_pp_process_enabled_no_nodes_copies_frame_unchanged(void)
{
    pp_init(&pp, true, &ctx);
    push_raw_frame(1.5f, 1000u, 0u);

    TBCI_NodeResult r = pp_process(&pp, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, r);

    float       out[SIG_CHANNELS];
    TBCI_Frame  meta;
    TBCI_Status s = sb_peek_latest(&proc_buf, out, &meta);
    TEST_ASSERT_EQUAL(TBCI_OK, s);

    float expected[SIG_CHANNELS] = {1.5f, 1.5f, 1.5f, 1.5f};
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, out, SIG_CHANNELS);
    TEST_ASSERT_EQUAL_UINT64(1000u, meta.timestamp_us);
    TEST_ASSERT_EQUAL_UINT32(0u, meta.sample_index);
}

void test_pp_process_disabled_still_copies_frame_unchanged(void)
{
    pp_init(&pp, false, &ctx);
    push_raw_frame(2.0f, 2000u, 1u);

    TBCI_NodeResult r = pp_process(&pp, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, r);

    float       out[SIG_CHANNELS];
    TBCI_Frame  meta;
    TBCI_Status s = sb_peek_latest(&proc_buf, out, &meta);
    TEST_ASSERT_EQUAL(TBCI_OK, s);

    float expected[SIG_CHANNELS] = {2.0f, 2.0f, 2.0f, 2.0f};
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, out, SIG_CHANNELS);
    TEST_ASSERT_EQUAL_UINT64(2000u, meta.timestamp_us);
}

void test_pp_process_multiple_ticks_grows_processed_buffer(void)
{
    pp_init(&pp, true, &ctx);

    push_raw_frame(1.0f, 1000u, 0u);
    pp_process(&pp, &ctx);

    push_raw_frame(2.0f, 1004u, 1u);
    pp_process(&pp, &ctx);

    TEST_ASSERT_EQUAL_size_t(2, sb_size(&proc_buf));

    float       out[SIG_CHANNELS];
    TBCI_Frame  meta;
    sb_peek_latest(&proc_buf, out, &meta);

    float expected[SIG_CHANNELS] = {2.0f, 2.0f, 2.0f, 2.0f};
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, out, SIG_CHANNELS);
    TEST_ASSERT_EQUAL_UINT64(1004u, meta.timestamp_us);
}

/* ============================================================
 * GROUP 4 — pp_process: with one inner node
 *
 * Inner nodes must be registered BEFORE pp_init, since pp_init
 * calls group_init on whatever is registered at that time.
 * ============================================================ */

void test_pp_process_enabled_with_node_applies_transform(void)
{
    group_add_node(&pp.group, &double_node);
    pp_init(&pp, true, &ctx);

    push_raw_frame(3.0f, 3000u, 2u);

    TBCI_NodeResult r = pp_process(&pp, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, r);

    float       out[SIG_CHANNELS];
    TBCI_Frame  meta;
    sb_peek_latest(&proc_buf, out, &meta);

    float expected[SIG_CHANNELS] = {6.0f, 6.0f, 6.0f, 6.0f}; /* 3.0 * 2 */
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, out, SIG_CHANNELS);
}

void test_pp_process_disabled_group_skips_node_transform(void)
{
    group_add_node(&pp.group, &double_node);
    pp_init(&pp, false, &ctx);

    push_raw_frame(3.0f, 3000u, 2u);

    TBCI_NodeResult r = pp_process(&pp, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, r);

    float       out[SIG_CHANNELS];
    TBCI_Frame  meta;
    sb_peek_latest(&proc_buf, out, &meta);

    /* group disabled -> node not applied -> raw value preserved */
    float expected[SIG_CHANNELS] = {3.0f, 3.0f, 3.0f, 3.0f};
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, out, SIG_CHANNELS);
}

/* ============================================================
 * GROUP 5 — pp_reset
 * ============================================================ */

void test_pp_reset_null_returns_invalid_arg(void)
{
    TBCI_Status s = pp_reset(NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_pp_reset_calls_inner_node_reset(void)
{
    group_add_node(&pp.group, &double_node);
    pp_init(&pp, true, &ctx);

    TBCI_Status s = pp_reset(&pp);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
    TEST_ASSERT_EQUAL(1, reset_calls);
}

int main(void)
{
    UNITY_BEGIN();

    // Group 1 — pp_init
    RUN_TEST(test_pp_init_null_node_returns_invalid_arg);
    RUN_TEST(test_pp_init_null_ctx_returns_invalid_arg);
    RUN_TEST(test_pp_init_sets_type);
    RUN_TEST(test_pp_init_enabled_flag_set);
    RUN_TEST(test_pp_init_n_nodes_zero_by_default);
    RUN_TEST(test_pp_init_preserves_caller_registered_nodes);
    RUN_TEST(test_pp_init_calls_init_on_registered_nodes);

    // Group 2 — pp_process validation
    RUN_TEST(test_pp_process_null_node_returns_error);
    RUN_TEST(test_pp_process_null_ctx_returns_error);
    RUN_TEST(test_pp_process_empty_signal_buffer_returns_pending);

    // Group 3 — pass-through
    RUN_TEST(test_pp_process_enabled_no_nodes_copies_frame_unchanged);
    RUN_TEST(test_pp_process_disabled_still_copies_frame_unchanged);
    RUN_TEST(test_pp_process_multiple_ticks_grows_processed_buffer);

    // Group 4 — with inner node
    RUN_TEST(test_pp_process_enabled_with_node_applies_transform);
    RUN_TEST(test_pp_process_disabled_group_skips_node_transform);

    // Group 5 — pp_reset
    RUN_TEST(test_pp_reset_null_returns_invalid_arg);
    RUN_TEST(test_pp_reset_calls_inner_node_reset);

    return UNITY_END();
}